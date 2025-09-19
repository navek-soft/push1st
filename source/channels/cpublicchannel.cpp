#include "channels/cpublicchannel.h"

#include "channels/cchannels.h"
#include "core/cluster/ccluster.h"
#include "core/subscriber/csubscriber.h"
#include "log/clog.h"

size_t cpublicchannel::Push(message_t&& msg) {
    Cluster->Push(chType, chApp, chName, *msg);
    return cchannel::Push(std::move(msg));
}

void cpublicchannel::Subscribe(const std::shared_ptr<csubscriber>& subscriber) {
    {
        std::unique_lock<decltype(chSubscribersLock)> lock(chSubscribersLock);
        chSubscribers.emplace(subscriber->Id(), std::dynamic_pointer_cast<csubscriber>(subscriber));
    }
    PSHT_INFO(
        "Subscribe {}:{}:{} ( {} sessions)", subscriber->GetFd(), chUid.c_str(), subscriber->Id().c_str(), chSubscribers.size());
    chApp->Trigger(chType, hook_t::type::join, chName, subscriber->Id(), {});
    Cluster->Trigger(chType, hook_t::type::join, chApp, chName, subscriber->Id(), {});
}

void cpublicchannel::OnSubscriberLeave(const std::string& subscriber) {
    PSHT_INFO("UnSubscribe {}:{} ( {} sessions)", chUid.c_str(), subscriber.c_str(), chSubscribers.size());
    chApp->Trigger(chType, hook_t::type::leave, chName, subscriber, {});
    Cluster->Trigger(chType, hook_t::type::leave, chApp, chName, subscriber, {});
}

void cpublicchannel::UnSubscribe(const std::string& subscriber) {
    {
        std::unique_lock<decltype(chSubscribersLock)> lock(chSubscribersLock, std::defer_lock);
        if (lock.try_lock()) {
            chSubscribers.erase(subscriber);
            if (chSubscribers.empty() and chMode == autoclose_t::yes) {
                chChannels->UnRegister(chUid);
            }
        }
    }
    PSHT_INFO("UnSubscribe {}:{} ( {} sessions)", chUid.c_str(), subscriber.c_str(), chSubscribers.size());
    chApp->Trigger(chType, hook_t::type::leave, chName, subscriber, {});
    Cluster->Trigger(chType, hook_t::type::leave, chApp, chName, subscriber, {});
}

cpublicchannel::cpublicchannel(const std::shared_ptr<cchannels>& channels, const std::shared_ptr<ccluster>& cluster, const std::string& cuid, const std::string& name, const app_t& app, autoclose_t mode) :
    cchannel(channels, cuid, name, app, channel_t::type::pub, mode),
    Cluster {cluster} {
    PSHT_INFO("Register {}", chUid.c_str());
    chApp->Trigger(chType, hook_t::type::reg, chName, {}, {});
    Cluster->Trigger(chType, hook_t::type::reg, chApp, chName, {}, {});
}

cpublicchannel::~cpublicchannel() {
    PSHT_INFO("UnRegister {}", chUid.c_str());
    chApp->Trigger(chType, hook_t::type::unreg, chName, {}, {});
    Cluster->Trigger(chType, hook_t::type::unreg, chApp, chName, {}, {});
}