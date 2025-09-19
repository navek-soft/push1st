#pragma once
#include <log/clog.h>

#include "cchannel.h"

class ccluster;

class cpresencechannel : public cchannel, public std::enable_shared_from_this<cpresencechannel> {
    log_as(presencechannel);

   public:
    cpresencechannel(const std::shared_ptr<cchannels>& channels, const std::shared_ptr<ccluster>& cluster, const std::string& cuid, const std::string& name, const app_t& app, autoclose_t mode);
    ~cpresencechannel() override;
    size_t Push(message_t&& msg) override;
    void Subscribe(const std::shared_ptr<csubscriber>& subscriber) override;
    void UnSubscribe(const std::string& subscriber) override;
    void GetUsers(usersids_t&, userslist_t&) override;
    void OnClusterJoin(const json::value_t& val) override;
    void OnClusterLeave(const json::value_t& val) override;

   protected:
    void OnSubscriberLeave(const std::string& subscriber) override;

   private:
    std::shared_ptr<ccluster> Cluster;
    std::shared_mutex UsersLock;
    std::unordered_map<std::string /* sess */, std::pair<std::string /* user */, std::string /* info */>> UsersList;
};