#pragma once
#include <map>
#include <set>
#include <string>
#include <cinttypes>

namespace inet {
	class ciface {
	public:
		std::string name;
#pragma pack(push,1)
		struct {
			uint32_t up : 1;
			uint32_t broadcast : 1;
			uint32_t debug : 1;
			uint32_t loopback : 1;
			uint32_t pointopoint : 1;
			uint32_t notrailers : 1;
			uint32_t running : 1;
			uint32_t noarp : 1;
			uint32_t promisc : 1;
			uint32_t allmulti : 1;
			uint32_t master : 1;
			uint32_t slave : 1;
			uint32_t multicast : 1;
			uint32_t portsel : 1;
			uint32_t automedia : 1;
			uint32_t dynamic : 1;
			uint32_t : 16;
		} flags;
#pragma pack(pop)
		size_t		mtu;
		size_t		index;
		struct {
			uint32_t	address;
			uint32_t	netmask;
			inline bool empty() const { return address == 0; }
		} ip4;
		struct {
			uint8_t		address[16];
			uint8_t		netmask[16];
			inline bool empty() const { auto&& a64 = reinterpret_cast<const uint64_t*>(address); return (a64[0] + a64[1]) == 0; }
		} ip6;
		struct {
			uint8_t		address[6];
			inline bool empty() const { auto&& a16 = reinterpret_cast<const uint16_t*>(address); return (a16[0] + a16[1] + a16[2]) == 0; }
		} mac;
		ciface(const char* iface_name);
	};

	std::map<std::string, ciface> GetNetInterfaces();
	std::set<uint32_t> GetLocalAddresses();

	bool IsLocalIp(uint32_t ip);
}