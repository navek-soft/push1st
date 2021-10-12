#include "cchannel.h"
#include "../cmessage.h"
#include <forward_list>
#include "../csubscriber.h"
#include "../cchannels.h"

json::value_t cchannel::ApiStats() {
	return json::object_t{
		{"channel", chName },
		{"type", str(chType)},
		{"close",chMode == autoclose_t::yes ? "auto" : "manual"},
		{"sess_count",chSubscribers.size()}
	};
}

json::value_t cchannel::ApiOverview() {
	json::array_t sess;
	{
		std::shared_lock<decltype(chSubscribersLock)> lock;
		for (auto&& [sid, ses] : chSubscribers) {
			sess.emplace_back(sid);
		}
	}
	return json::object_t{
		{"channel", chName },
		{"type", str(chType)},
		{"close",chMode == autoclose_t::yes ? "auto" : "manual"},
		{"sessions",sess}
	};
}

size_t cchannel::Push(message_t&& message) {
	std::forward_list<std::string> sessLeave;
	size_t nSubscribers{ 0 };

	msg::delivery_t delivery{ (*message)["#msg-delivery"].get<std::string_view>() == "broadcast" ? msg::delivery_t::broadcast :
		((*message)["#msg-delivery"].get<std::string_view>() == "multicast" ? msg::delivery_t::multicast : msg::delivery_t::unicast) };

	if (delivery == msg::delivery_t::broadcast) {
		std::shared_lock<decltype(chSubscribersLock)> lock(chSubscribersLock);
		for (auto&& [sess, subs] : chSubscribers) {
			if (sess != (*message)["#msg-from"].get<std::string_view>()) {
				if (auto&& subsSelf{ subs.lock() }; subsSelf) {
					subsSelf->Push(message);
					++nSubscribers;
					if (delivery == msg::delivery_t::broadcast) continue;
					break;
				}
				else {
					sessLeave.emplace_front(sess);
				}
			}
		}
	}
	else if((*message).contains("socket_id") and !(*message)["socket_id"].empty()) {
		std::string SessionId{ (*message)["socket_id"] };
		if (delivery == msg::delivery_t::multicast) {
			SessionId.push_back('.');
			std::shared_lock<decltype(chSubscribersLock)> lock(chSubscribersLock);
			for (auto&& [sess, subs] : chSubscribers) {
				if (sess.compare(0, SessionId.length(), SessionId) == 0) {
					if (auto&& subsSelf{ subs.lock() }; subsSelf) {
						subsSelf->Push(message);
						++nSubscribers;
					}
					else {
						sessLeave.emplace_front(sess);
					}
				}
			}
		}
		else {
			std::shared_lock<decltype(chSubscribersLock)> lock(chSubscribersLock);
			if (auto&& subs{ chSubscribers.find(SessionId) }; subs != chSubscribers.end()) {
				if (auto&& subsSelf{ subs->second.lock() }; subsSelf) {
					subsSelf->Push(message);
					++nSubscribers;
				}
				else {
					sessLeave.emplace_front(subs->first);
				}
			}
		}
	}
	if (!sessLeave.empty()) {
		while (!sessLeave.empty()) {
			{
				std::unique_lock<decltype(chSubscribersLock)> lock(chSubscribersLock);
				chSubscribers.erase(sessLeave.front());
			}
			OnSubscriberLeave(sessLeave.front());
			sessLeave.pop_front();
		}

		std::unique_lock<decltype(chSubscribersLock)> lock(chSubscribersLock);
		if (chSubscribers.empty() and chMode == autoclose_t::yes) {
			chChannels->UnRegister(chUid);
		}
	}
	return nSubscribers;
}


cchannel::cchannel(const std::shared_ptr<cchannels>& channels, const std::string& cuid, const std::string& name, const app_t& app, channel_t type, autoclose_t mode) :
	chType{ type }, chMode{ mode }, chUid{ cuid }, chName{ name }, chApp{ app }, chChannels{ channels }
{
}

cchannel::~cchannel() {

}
