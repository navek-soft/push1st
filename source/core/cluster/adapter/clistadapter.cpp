#include "core/cluster/adapters/clistadapter.h"

#include <memory>

#include "inet/ciface.h"

clistadapter::clistadapter(const std::unordered_set<std::string>& nodes, const std::string_view& port) {
    for (auto&& nIt : nodes) {
        if (!nIt.empty()) {
            if (std::unique_ptr<node_t> node {std::make_unique<node_t>()}; node and inet::GetSockAddr(node->NodeAddress, nIt, port, AF_INET) == 0) {
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
}