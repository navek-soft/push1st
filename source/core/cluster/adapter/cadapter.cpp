#include "core/cluster/adapters/cadapter.h"

#include "log/clog.h"

void cadapter::UpdatePeer(struct sockaddr_storage& sa) {
    auto ip {inet::GetIp4(sa)};
    std::unique_lock _(clusLock);
    if (auto&& it {clusNodes.find(ip)}; it != clusNodes.end()) {
        it->second->NodeLastActivity = std::time(nullptr);
        PSHT_DEBUG("Node {}:{} ... PING", inet::GetIp(sa).c_str(), inet::GetPort(sa));
    } else if (std::unique_ptr<node_t> node {new node_t}; node) {
        node->NodeIp = ip;
        node->NodeLastActivity = std::time(nullptr);
        std::memcpy(&node->NodeAddress, &sa, sizeof(struct sockaddr_storage));
        clusNodes.emplace(ip, std::move(node));
        PSHT_DEBUG("Node {}:{} ... ADD ( PING )", inet::GetIp(sa).c_str(), inet::GetPort(sa));
    }
}

ssize_t cadapter::Gc(std::time_t now, std::time_t pingInterval) {
    ssize_t deleted = 0;
    std::unique_lock _(clusLock);
    PSHT_TRACE("Cluster Gc... ()", now);
    for (auto it = clusNodes.begin(); it != clusNodes.end();) {
        auto& node = it->second;
        if (node->NodeLastActivity + pingInterval + 10 < now) {
            auto& sa = node->NodeAddress;
            PSHT_DEBUG("Node {}:{} ... DELETED ({} < {})", inet::GetIp(sa).c_str(), inet::GetPort(sa), node->NodeLastActivity, now);
            it = clusNodes.erase(it);
            ++deleted;
        } else {
            ++it;
        }
    }
    return deleted;
}

#include "core/cluster/adapters/ck8sadapter.h"
#include "core/cluster/adapters/clistadapter.h"

void MakeInterface(std::unique_ptr<cadapter>& adapter, config::cluster_t& config) {
    switch (config.Type) {
        case cluster::type_t::peers: {
            adapter = clistadapter::MakeUnique(config.Nodes, config.Listen.Port());
            break;
        }
        case cluster::type_t::k8s: {
            adapter = ck8sadapter::MakeUnique(
                std::make_shared<inet::cpoll>("k8s-worker"), config.Url, config.Namespace, config.Ssl.Context());
            break;
        }
        default:
            return;
    };
}