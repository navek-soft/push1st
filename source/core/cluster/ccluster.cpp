#include "core/cluster/ccluster.h"

#include <unistd.h>

#include "broker/api.h"
#include "core/credentials/ccredentials.h"
#include "core/util/cjson.h"
#include "inet/ciface.h"
#include "inet/cinet.h"
#include "log/clog.h"

namespace proto {
enum class op_t : uint8_t { noop = 0, ping = 1, reg = 2, unreg = 3, join = 4, leave = 5, push = 6 };
static const size_t MaxFrameSize {65400};
#pragma pack(push, 1)
struct hdr_t {
    op_t op : 4;
    uint8_t fr : 1; /* fragmented flag */
    uint8_t sf : 1; /* start fragment */
    uint8_t lt : 2; /* length type : 1 (short), 2 (long), 3 (large) */
    union {
        struct {
            uint16_t len;
            uint8_t payload[0];
        } sh;
    } data;
};
#pragma pack(pop)

inline array_t Pack(op_t op, json::object_t&& msg) {
    auto&& packedData {json::Serialize(msg)};
    array_t frame {nullptr, packedData.size() + sizeof(hdr_t)};
    if (frame.first = std::shared_ptr<uint8_t[]>(new uint8_t[frame.second]); frame.first) {
        auto hdr {(hdr_t*)frame.first.get()};
        hdr->fr = 0;
        hdr->sf = 1;
        hdr->lt = 1;
        hdr->op = op;
        hdr->data.sh.len = (uint16_t)packedData.size();
        std::memcpy(hdr->data.sh.payload, packedData.data(), packedData.size());
        return frame;
    }
    return {nullptr, 0};
}
inline array_t Pack(hook_t::type trigger, json::object_t&& msg) {
    switch (trigger) {
        case hook_t::type::push:
            return Pack(op_t::push, std::move(msg));
        case hook_t::type::join:
            return Pack(op_t::join, std::move(msg));
        case hook_t::type::leave:
            return Pack(op_t::leave, std::move(msg));
        case hook_t::type::reg:
            return Pack(op_t::reg, std::move(msg));
        case hook_t::type::unreg:
            return Pack(op_t::unreg, std::move(msg));
        default:
            break;
    }
    return {nullptr, 0};
}
}// namespace proto

inline void ccluster::OnClusterPing(struct sockaddr_storage& sa, const std::string_view& data) {
    auto ip {inet::GetIp4(sa)};
    std::unique_lock<decltype(clusLock)> lock(clusLock);
    if (auto&& it {clusNodes.find(ip)}; it != clusNodes.end()) {
        it->second->NodeLastActivity = std::time(nullptr);

        PSHT_INFO("Node {}:{} ... PING", inet::GetIp(sa).c_str(), inet::GetPort(sa));
    } else if (std::unique_ptr<cnode> node {new cnode}; node) {
        node->NodeIp = ip;
        node->NodeLastActivity = std::time(nullptr);
        std::memcpy(&node->NodeAddress, &sa, sizeof(struct sockaddr_storage));
        clusNodes.emplace(ip, std::move(node));
        PSHT_INFO("Node {}:{} ... ADD ( PING )", inet::GetIp(sa).c_str(), inet::GetPort(sa));
    }
    CallModule("OnClusterPing", {inet::GetIp(sa), data});
}

inline void ccluster::OnClusterReg(struct sockaddr_storage& sa, const std::string_view& data) {
    PSHT_INFO("Node {}:{} ... REG", inet::GetIp(sa).c_str(), inet::GetPort(sa));
    CallModule("OnClusterRegister", {inet::GetIp(sa), data});
}

inline void ccluster::OnClusterUnReg(struct sockaddr_storage& sa, const std::string_view& data) {
    PSHT_INFO("Node {}:{} ... UNREG", inet::GetIp(sa).c_str(), inet::GetPort(sa));
    CallModule("OnClusterUnRegister", {inet::GetIp(sa), data});
}

inline void ccluster::OnClusterJoin(struct sockaddr_storage& sa, const std::string_view& data) {
    if (json::value_t value; json::Unserialize(data, value) and value.is_object() and value.contains("app") and value.contains("channel")
                             and value.contains("data")) {
        PSHT_INFO("Node {}:{} ... JOIN", inet::GetIp(sa).c_str(), inet::GetPort(sa));
        if (auto&& ch {Broker->GetChannel(value["app"].get<std::string>(), value["channel"].get<std::string>())}; ch) {
            ch->OnClusterJoin(value["data"]);
        }
        CallModule("OnClusterJoin", {inet::GetIp(sa), data});
    } else {
        PSHT_ERROR("Node {} ... JOIN ( invalid message )", inet::GetIp(sa).c_str());
    }
}

inline void ccluster::OnClusterLeave(struct sockaddr_storage& sa, const std::string_view& data) {
    if (json::value_t value; json::Unserialize(data, value) and value.is_object() and value.contains("app") and value.contains("channel")
                             and value.contains("data")) {
        PSHT_INFO("Node {}:{} ... LEAVE", inet::GetIp(sa).c_str(), inet::GetPort(sa));
        if (auto&& ch {Broker->GetChannel(value["app"].get<std::string>(), value["channel"].get<std::string>())}; ch) {
            ch->OnClusterLeave(value["data"]);
        }
        CallModule("OnClusterLeave", {inet::GetIp(sa), data});
    } else {
        PSHT_ERROR("Node {} ... LEAVE ( invalid message )", inet::GetIp(sa).c_str());
    }
}

inline void ccluster::OnClusterPush(struct sockaddr_storage& sa, const std::string_view& data) {
    if (json::value_t value; json::Unserialize(data, value) and value.is_object() and value.contains("app") and value.contains("channel")
                             and value.contains("data")) {
        PSHT_INFO("Node {} ... PUSH", inet::GetIp(sa).c_str());
        if (auto&& ch {Broker->GetChannel(value["app"].get<std::string>(), value["channel"].get<std::string>())}; ch) {
            value["data"]["#from-host"] = inet::GetIp(sa);
            ch->OnClusterPush(message_t {std::make_shared<json::value_t>(value["data"])});
        }
        CallModule("OnClusterPush", {inet::GetIp(sa), data});
    } else {
        PSHT_ERROR("Node {} ... PUSH ( invalid message )", inet::GetIp(sa).c_str());
    }
}

inline void ccluster::Send(data_t data) {
    if (data.first and data.second <= proto::MaxFrameSize) {
        std::unique_lock<decltype(clusLock)> lock(clusLock);
        size_t nwrite {0};
        ssize_t res {0};
        for (auto&& [ip, node] : clusNodes) {
            /*
            int cliFd{ -1 };
            if (inet::UdpConnect(cliFd, node->NodeAddress, false) == 0) {
                res = ::send(cliFd, data.first.get(), data.second, MSG_NOSIGNAL);
                ::close(cliFd);
                PSHT_INFO("Send to `{}:{}` ( {} )", inet::GetIp(node->NodeAddress).c_str(),
            be16toh(inet::GetPort(node->NodeAddress)), nwrite, std::strerror(-(int)res)); continue;
            }
            */
            if (res = clusFd.SocketSend(node->NodeAddress, data.first.get(), data.second, nwrite, 0); res == 0) {
                PSHT_INFO("Send to `{}:{}` ( {} )", inet::GetIp(node->NodeAddress).c_str(), be16toh(inet::GetPort(node->NodeAddress)), nwrite, std::strerror(-(int)res));
                continue;
            }

            PSHT_ERROR("Send to `{}` error ( {} )", inet::GetIp(node->NodeAddress).c_str(), std::strerror(-(int)res));
        }
    } else {
        PSHT_ERROR("Send error ( {} )", "Message to long");
    }
}

ssize_t ccluster::OnUdpData(fd_t fd, const inet::ssl_t& ssl) {
    if (inet::csocket so {fd, ssl}; so) {
        struct sockaddr_storage sa;
        ssize_t nread {0};
        uint8_t* data {(uint8_t*)alloca(proto::MaxFrameSize)};
        proto::hdr_t* frame {(proto::hdr_t*)data};

        ssize_t res {-1};

        if (res = so.SocketRecv(sa, data, sizeof(proto::hdr_t), (size_t&)nread, MSG_PEEK); res == 0 and nread == sizeof(proto::hdr_t)) {
            if (frame->data.sh.len < (proto::MaxFrameSize - sizeof(proto::hdr_t))) {
                if (res = so.SocketRecv(sa, data, sizeof(proto::hdr_t) + frame->data.sh.len, (size_t&)nread, MSG_WAITALL); res == 0) {
                    switch (frame->op) {
                        case proto::op_t::push:
                            OnClusterPush(sa, {(char*)frame->data.sh.payload, frame->data.sh.len});
                            break;
                        case proto::op_t::ping:
                            OnClusterPing(sa, {(char*)frame->data.sh.payload, frame->data.sh.len});
                            break;
                        case proto::op_t::reg:
                            OnClusterReg(sa, {(char*)frame->data.sh.payload, frame->data.sh.len});
                            break;
                        case proto::op_t::unreg:
                            OnClusterUnReg(sa, {(char*)frame->data.sh.payload, frame->data.sh.len});
                            break;
                        case proto::op_t::join:
                            OnClusterJoin(sa, {(char*)frame->data.sh.payload, frame->data.sh.len});
                            break;
                        case proto::op_t::leave:
                            OnClusterLeave(sa, {(char*)frame->data.sh.payload, frame->data.sh.len});
                            break;
                        default:
                            PSHT_ERROR("Network error ( {} )", std::strerror(EBADMSG));
                            return -EAGAIN;
                    }
                } else {
                    PSHT_ERROR("Network error ( {} )", std::strerror(EMSGSIZE));
                    return -EAGAIN;
                }
            } else {
                PSHT_ERROR("Network error ( {} )", std::strerror(-(int)res));
                return res;
            }
        } else {
            PSHT_ERROR("Network error ( {} )", std::strerror(-(int)res));
            return res;
        }
    } else {
        PSHT_ERROR("Network error ( {} )", std::strerror(EBADFD));
        return -EBADFD;
    }

    return 0;
}

void ccluster::Push(channel_t::type type, const app_t& app, sid_t channel, const json::object_t& data) {
    if (app->IsAllowTrigger(type, hook_t::type::push)) {
        //(*msg)["#msg-from"] = "cluster";
        Send(proto::Pack(hook_t::type::push, {{"app", app->Id}, {"channel", channel}, {"data", json::Serialize(data)}}));
    }
}

void ccluster::Trigger([[maybe_unused]] channel_t::type type, hook_t::type trigger, const app_t& app, sid_t channel, [[maybe_unused]] sid_t session, const json::object_t& data) {
    if (clusSync & trigger) {
        Send(proto::Pack(trigger, {{"app", app->Id}, {"channel", channel}, {"data", json::Serialize(data)}}));
    }
}

void ccluster::Ping() {
    if (clusPingInterval) {
        if (auto now = std::time(nullptr); now >= clusPingTime) {
            clusPingTime = now + clusPingInterval;
            Send(proto::Pack(proto::op_t::ping, json::object_t {}));
        }
    }
}

inline void ccluster::CallModule(const std::string& method, std::vector<std::any>&& args) {
    if (clusModuleAllowed) {
        jitLua.luaExecute(clusModule, method, args);
    }
}

ccluster::ccluster(const broker_t& broker, config::cluster_t& config) :
    inet::cudpserver("clus:srv"),
    Broker {broker},
    clusPingInterval {config.PingInterval},
    clusSync {config.Sync},
    clusModuleAllowed {std::filesystem::exists(config.Module.Path())},
    clusModule {config.Module.Path()} {
    if (config.Enable) {
        PSHT_INFO("Cluster enable, listen on {}", std::string {config.Listen.HostPort()}.c_str());

        for (auto&& nIt : config.Nodes) {
            if (!nIt.empty()) {
                if (std::unique_ptr<cnode> node {new cnode};
                    node and inet::GetSockAddr(node->NodeAddress, nIt, config.Listen.Port(), AF_INET) == 0) {
                    node->NodeIp = inet::GetIp4(node->NodeAddress);
                    if (!inet::IsLocalIp(node->NodeIp)) {
                        PSHT_INFO("Node {} ... enable, address {}", nIt.c_str(), inet::GetIp(node->NodeAddress).c_str());
                        clusNodes.emplace(node->NodeIp, std::move(node));
                    } else {
                        PSHT_INFO("Node {} ... I'm, address {}", nIt.c_str(), inet::GetIp(node->NodeAddress).c_str());
                    }
                } else {
                    PSHT_ERROR("Node {} ... disabled ( address not resolved )", nIt.c_str());
                }
            }
        }

        if (auto res = UdpListen(config.Listen.HostPort(), true, true, false); res == 0) {
            clusFd = Fd();
            // inet::SetUdpCork(clusFd.Fd(), false);
            /*
            if (int cliSo{ -1 }; inet::UdpConnect(cliSo, false) == 0) {
                cliFd = cliSo;
                //inet::SetUdpCork(cliFd.Fd(), true);
            }
            */
        } else {
            PSHT_ERROR("Cluster initialize server error ( {} )", std::strerror(-(int)res));
            throw std::runtime_error("Cluster server exception");
        }
    } else {
        PSHT_WARNING("Cluster disable");
    }
}

ccluster::~ccluster() {}
