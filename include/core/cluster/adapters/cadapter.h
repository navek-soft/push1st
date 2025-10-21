#pragma once

#include <log/clog.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstdint>
#include <memory>
#include <unordered_map>

#include "core/util/cspinlock.h"

class cadapter {
    log_as(adapter);

   protected:
    struct node_t {
        node_t() = default;
        node_t(uint32_t ip, sockaddr_storage sa) : NodeIp {ip}, NodeAddress(sa) {}

        uint32_t NodeIp {0};
        std::time_t NodeLastActivity {0};
        struct sockaddr_storage NodeAddress;
    };

    cadapter() = default;

   public:
    virtual ~cadapter() = default;
    virtual void Start() = 0;
    virtual void Check() {}

   public:
    void UpdatePeer(struct sockaddr_storage& sa);
    ssize_t Gc(std::time_t now, std::time_t pingInterval);

    template <class Func>
    void ProcessPeers(const Func& func) {
        std::unique_lock<decltype(clusLock)> lock(clusLock);
        for (auto&& [ip, node] : clusNodes) {
            func(ip, node);
        }
    }

   protected:
    core::cspinlock clusLock;
    std::unordered_map<uint32_t /* ip */, std::unique_ptr<node_t>> clusNodes;
};

void MakeInterface(std::unique_ptr<cadapter>& adapter, config::cluster_t& config);
