#include "inet/ciface.h"

#include <ifaddrs.h>
#include <linux/if_link.h>
#include <net/if.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <cstring>

namespace inet {

bool IsLocalIp(uint32_t ip) {
    static const std::set<uint32_t> LocalIpList {(GetLocalAddresses())};
    return (LocalIpList.contains(ip) or (ip & htonl(0x7f000000)) == htonl(0x7f000000));
}

std::set<uint32_t> GetLocalAddresses() {
    std::set<uint32_t> list;
    struct ifaddrs* ifaddr;
    if (getifaddrs(&ifaddr) != -1) {
        for (struct ifaddrs* ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next) {
            if (ifa->ifa_name) {
                if (ifa->ifa_addr) {
                    if (ifa->ifa_addr->sa_family == AF_INET) {
                        list.emplace(reinterpret_cast<struct sockaddr_in*>(ifa->ifa_addr)->sin_addr.s_addr);
                    } else if (ifa->ifa_addr->sa_family == AF_INET6) {
                        list.emplace(reinterpret_cast<struct sockaddr_in6*>(ifa->ifa_addr)->sin6_addr.__in6_u.__u6_addr32[3]);
                    }
                }
            }
        }
        freeifaddrs(ifaddr);
    }
    return list;
}

std::map<std::string, ciface> GetNetInterfaces() {
    std::map<std::string, ciface> list;
    struct ifaddrs* ifaddr;
    if (getifaddrs(&ifaddr) != -1) {
        for (struct ifaddrs* ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next) {
            if (ifa->ifa_name) {
                auto&& it = list.emplace(ifa->ifa_name, ciface(ifa->ifa_name));

                if (ifa->ifa_addr) {
                    if (ifa->ifa_addr->sa_family == AF_INET) {
                        it.first->second.ip4.address =
                            ntohl(reinterpret_cast<struct sockaddr_in*>(ifa->ifa_addr)->sin_addr.s_addr);
                        if (ifa->ifa_netmask) {
                            it.first->second.ip4.netmask =
                                ntohl(reinterpret_cast<struct sockaddr_in*>(ifa->ifa_netmask)->sin_addr.s_addr);
                        }
                    } else if (ifa->ifa_addr->sa_family == AF_INET6) {
                        memcpy(it.first->second.ip6.address,
                               reinterpret_cast<struct sockaddr_in6*>(ifa->ifa_addr)->sin6_addr.__in6_u.__u6_addr8,
                               sizeof(it.first->second.ip6.address));
                        if (ifa->ifa_netmask) {
                            memcpy(it.first->second.ip6.netmask,
                                   reinterpret_cast<struct sockaddr_in6*>(ifa->ifa_netmask)->sin6_addr.__in6_u.__u6_addr8,
                                   sizeof(it.first->second.ip6.netmask));
                        }
                    }
                    /*
                    else if (ifa->ifa_addr->sa_family == AF_PACKET && ifa->ifa_data != nullptr) {
                        struct rtnl_link_stats *stats = (struct rtnl_link_stats *)ifa->ifa_data;
                    }
                    */
                    //	*(uint32_t*)(&it.first->second.flags) = (uint32_t)ifa->ifa_flags;
                    std::memcpy(&it.first->second.flags, &ifa->ifa_flags, sizeof(uint32_t));
                }

                {
                    auto sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_IP);
                    if (sock > 0) {
                        struct ifreq ifr;

                        strncpy(ifr.ifr_name, ifa->ifa_name, sizeof(ifr.ifr_name));
                        if (it.first->second.mac.Empty() && ioctl(sock, SIOCGIFHWADDR, &ifr) == 0) {
                            memcpy(it.first->second.mac.address, ifr.ifr_addr.sa_data, sizeof(it.first->second.mac.address));
                        }
                        if (!it.first->second.mtu && ioctl(sock, SIOCGIFMTU, &ifr) == 0) {
                            it.first->second.mtu = ifr.ifr_mtu;
                        }
                        if (ioctl(sock, SIOCGIFINDEX, &ifr) == 0) {
                            it.first->second.index = ifr.ifr_ifindex;
                        }
                        ::close(sock);
                    }
                }
            }
        }
        freeifaddrs(ifaddr);
    }
    return list;
}

ciface::ciface(const char* iface_name) :
    name(iface_name),
    flags({0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}),
    ip4({0, 0}),
    ip6({{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}}),
    mac({0, 0, 0, 0, 0, 0}) {}

}// namespace inet