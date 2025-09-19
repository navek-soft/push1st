#include "channels/cprivatechannel.h"

#include "channels/cchannels.h"
#include "core/cluster/ccluster.h"
#include "core/subscriber/csubscriber.h"
#include "log/clog.h"

size_t cprivatechannel::Push(message_t&& msg) {
    Cluster->Push(chType, chApp, chName, *msg);
    return cchannel::Push(std::move(msg));
}

void cprivatechannel::Subscribe(const std::shared_ptr<csubscriber>& subscriber) {
    {
        std::unique_lock<decltype(chSubscribersLock)> lock(chSubscribersLock);
        chSubscribers.emplace(subscriber->Id(), std::dynamic_pointer_cast<csubscriber>(subscriber));
    }

    PSHT_INFO("Subscribe {}:{} ( {} sessions)", chUid.c_str(), subscriber->Id().c_str(), chSubscribers.size());
    chApp->Trigger(chType, hook_t::type::join, chName, subscriber->Id(), {});
    Cluster->Trigger(chType, hook_t::type::join, chApp, chName, subscriber->Id(), {});
}

void cprivatechannel::OnSubscriberLeave(const std::string& subscriber) {
    PSHT_INFO("UnSubscribe {}:{} ( {} sessions)", chUid.c_str(), subscriber.c_str(), chSubscribers.size());
    chApp->Trigger(chType, hook_t::type::leave, chName, subscriber, {});
    Cluster->Trigger(chType, hook_t::type::leave, chApp, chName, subscriber, {});
}

void cprivatechannel::UnSubscribe(const std::string& subscriber) {
    {
        std::unique_lock<decltype(chSubscribersLock)> lock(chSubscribersLock, std::defer_lock);
        if (lock.try_lock()) {
            chSubscribers.erase(subscriber);
            if (chSubscribers.empty() and chMode == autoclose_t::yes) {
                chChannels->UnRegister(chUid);
            }
        }
    }
    chApp->Trigger(chType, hook_t::type::leave, chName, subscriber, {});
    Cluster->Trigger(chType, hook_t::type::leave, chApp, chName, subscriber, {});
    PSHT_INFO("UnSubscribe {}:{} ( {} sessions)\n", chUid.c_str(), subscriber.c_str(), chSubscribers.size());
}

cprivatechannel::cprivatechannel(const std::shared_ptr<cchannels>& channels, const std::shared_ptr<ccluster>& cluster, const std::string& cuid, const std::string& name, const app_t& app, autoclose_t mode) :
    cchannel(channels, cuid, name, app, channel_t::type::priv, mode),
    Cluster {cluster} {
    PSHT_INFO("Register {}", chUid.c_str());

    chApp->Trigger(chType, hook_t::type::reg, chName, {}, {});
    Cluster->Trigger(chType, hook_t::type::reg, chApp, chName, {}, {});
}

cprivatechannel::~cprivatechannel() {
    PSHT_INFO("UnRegister {}", chUid.c_str());
    chApp->Trigger(chType, hook_t::type::unreg, chName, {}, {});
    Cluster->Trigger(chType, hook_t::type::unreg, chApp, chName, {}, {});
}