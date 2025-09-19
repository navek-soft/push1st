#pragma once
#include <log/clog.h>

#include "cchannel.h"

class ccluster;

class cprivatechannel : public cchannel, public std::enable_shared_from_this<cprivatechannel> {
    log_as(privatechannel);

   public:
    cprivatechannel(const std::shared_ptr<cchannels>& channels, const std::shared_ptr<ccluster>& cluster, const std::string& cuid, const std::string& name, const app_t& app, autoclose_t mode);
    ~cprivatechannel() override;
    size_t Push(message_t&& msg) override;
    void Subscribe(const std::shared_ptr<csubscriber>& subscriber) override;
    void UnSubscribe(const std::string& subscriber) override;
    inline void GetUsers(usersids_t&, userslist_t&) override {
        ;
    }

   protected:
    void OnSubscriberLeave(const std::string& subscriber) override;

   private:
    std::shared_ptr<ccluster> Cluster;
};