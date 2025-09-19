#pragma once
#include <log/clog.h>

#include "cchannel.h"

class ccluster;
class cpublicchannel : public cchannel, public std::enable_shared_from_this<cpublicchannel> {
    log_as(publicchannel);

   public:
    cpublicchannel(const std::shared_ptr<cchannels>& channels, const std::shared_ptr<ccluster>& cluster, const std::string& cuid, const std::string& name, const app_t& app, autoclose_t mode);
    ~cpublicchannel() override;
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
