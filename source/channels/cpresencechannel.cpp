#include "channels/cpresencechannel.h"

#include "channels/cchannels.h"
#include "core/cluster/ccluster.h"
#include "core/subscriber/csubscriber.h"
#include "log/clog.h"

size_t cpresencechannel::Push(message_t&& msg) {
    Cluster->Push(chType, chApp, chName, *msg);
    return cchannel::Push(std::move(msg));
}

void cpresencechannel::GetUsers(usersids_t& ids, userslist_t& usrs) {
    decltype(UsersList) list;

    {
        std::unique_lock<decltype(UsersLock)> lock(UsersLock);
        list.insert(UsersList.begin(), UsersList.end());
    }

    for (auto&& [sess, info] : list) {
        auto&& [uId, uInfo] = info;
        ids.emplace(uId);
        usrs.emplace(uId, uInfo);
    }
}

void cpresencechannel::OnClusterJoin(const json::value_t& val) {
    if (val.is_object() and val.contains("user-id") and val.contains("user-info")) {
        cchannel::Push(
            msg::Make({{"event", "pusher_internal:member_added"},
                       {"channel", chName},
                       {"data", {{"user_id", val["user-id"].get<std::string>()}, {"user_info", val["user-info"].get<std::string>()}}}},
                      "presence"));
    }
}

void cpresencechannel::OnClusterLeave(const json::value_t& val) {
    if (val.is_object() and val.contains("user-id")) {
        cchannel::Push(msg::Make(
            {{"event", "pusher_internal:member_removed"}, {"channel", chName}, {"data", {{"user_id", val["user-id"].get<std::string>()}}}}, "presence"));
    }
}

void cpresencechannel::Subscribe(const std::shared_ptr<csubscriber>& subscriber) {
    {
        std::unique_lock<decltype(chSubscribersLock)> lock(chSubscribersLock);
        chSubscribers.emplace(subscriber->Id(), std::dynamic_pointer_cast<csubscriber>(subscriber));
    }

    std::string uId, uInfo;
    subscriber->GetUserInfo(uId, uInfo);
    if (!uId.empty()) {
        usersids_t ids;
        userslist_t lst;
        GetUsers(ids, lst);

        /* Push other subscriber itself */
        cchannel::Push(msg::Make({{"event", "pusher_internal:member_added"}, {"channel", chName}, {"data", {{"user_id", uId}, {"user_info", uInfo}}}}, "presence"));

        /* Self message pushing wit users list  */

        cchannel::Push(msg::Make({{"event", "pusher_internal:subscription_succeeded"},
                                  {"channel", chName},
                                  {"socket_id", json::array_t {subscriber->Id()}},
                                  {"data", {{"presence", {{"ids", ids}, {"hash", lst}, {"count", lst.size()}}}}}},
                                 "presence"));

        {
            std::unique_lock<decltype(UsersLock)> lock(UsersLock);
            UsersList.emplace(subscriber->Id(), std::make_pair(uId, uInfo));
        }
    }

    PSHT_INFO(
        "Subscribe {}:{} ( {} ) ( {} sessions)", chUid.c_str(), subscriber->Id().c_str(), uId.c_str(), chSubscribers.size());
    chApp->Trigger(chType, hook_t::type::join, chName, subscriber->Id(), {{"user-id", uId}, {"user-info", uInfo}});
    Cluster->Trigger(chType, hook_t::type::join, chApp, chName, subscriber->Id(), {{"user-id", uId}, {"user-info", uInfo}});
}

void cpresencechannel::OnSubscriberLeave(const std::string& subscriber) {
    std::string uId;
    {
        std::unique_lock<decltype(UsersLock)> lock(UsersLock);
        uId = UsersList[subscriber].first;
        UsersList.erase(subscriber);
    }
    if (!uId.empty()) {
        cchannel::Push(msg::Make({{"event", "pusher_internal:member_removed"}, {"channel", chName}, {"data", {{"user_id", uId}}}}, "presence"));
        chApp->Trigger(chType, hook_t::type::leave, chName, subscriber, {{"user-id", uId}});
        Cluster->Trigger(chType, hook_t::type::leave, chApp, chName, subscriber, {{"user-id", uId}});
    }
    PSHT_INFO("UnSubscribe {}:{} ( %ld sessions)\n", chUid.c_str(), subscriber.c_str(), chSubscribers.size());
}

void cpresencechannel::UnSubscribe(const std::string& subscriber) {
    {
        std::unique_lock<decltype(chSubscribersLock)> lock(chSubscribersLock, std::defer_lock);
        if (lock.try_lock()) {
            chSubscribers.erase(subscriber);
            if (chSubscribers.empty() and chMode == autoclose_t::yes) {
                chChannels->UnRegister(chUid);
            }
        }
    }
    {
        std::unique_lock<decltype(UsersLock)> lock(UsersLock, std::defer_lock);
        if (lock.try_lock()) {
            UsersList.erase(subscriber);
        }
    }
    PSHT_INFO("UnSubscribe {}:{} ( {} sessions)", chUid.c_str(), subscriber.c_str(), chSubscribers.size());
    chApp->Trigger(chType, hook_t::type::leave, chName, subscriber, {});
    Cluster->Trigger(chType, hook_t::type::leave, chApp, chName, subscriber, {});
}

cpresencechannel::cpresencechannel(const std::shared_ptr<cchannels>& channels, const std::shared_ptr<ccluster>& cluster, const std::string& cuid, const std::string& name, const app_t& app, autoclose_t mode) :
    cchannel(channels, cuid, name, app, channel_t::type::pres, mode),
    Cluster {cluster} {
    PSHT_INFO("Register {}", chUid.c_str());

    chApp->Trigger(chType, hook_t::type::reg, chName, {}, {});
    Cluster->Trigger(chType, hook_t::type::reg, chApp, chName, {}, {});
}

cpresencechannel::~cpresencechannel() {
    PSHT_INFO("UnRegister {}", chUid.c_str());
    chApp->Trigger(chType, hook_t::type::unreg, chName, {}, {});
    Cluster->Trigger(chType, hook_t::type::unreg, chApp, chName, {}, {});
}