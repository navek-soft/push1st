#include "core/cluster/adapters/cadapter.h"

void cadapter::UpdatePeer(struct sockaddr_storage& sa) {
    auto ip {inet::GetIp4(sa)};
    std::unique_lock _(clusLock);
    if (auto&& it {clusNodes.find(ip)}; it != clusNodes.end()) {
        it->second->NodeLastActivity = std::time(nullptr);
        PSHT_INFO("Node {}:{} ... PING", inet::GetIp(sa).c_str(), inet::GetPort(sa));
    } else if (std::unique_ptr<node_t> node {new node_t}; node) {
        node->NodeIp = ip;
        node->NodeLastActivity = std::time(nullptr);
        std::memcpy(&node->NodeAddress, &sa, sizeof(struct sockaddr_storage));
        clusNodes.emplace(ip, std::move(node));
        PSHT_INFO("Node {}:{} ... ADD ( PING )", inet::GetIp(sa).c_str(), inet::GetPort(sa));
    }
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