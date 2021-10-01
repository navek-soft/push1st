#include "cchannel.h"
#include "../cmessage.h"
#include <forward_list>
#include "../csubscriber.h"
#include "../cchannels.h"

size_t cchannel::Push(message_t&& message) {
	std::forward_list<std::string> sessLeave;
	auto&& msg{ msg::ref(message) };
	size_t nSubscribers{ 0 };

	msg::delivery_t delivery{ msg["#msg-delivery"].get<std::string_view>() == "broadcast" ? msg::delivery_t::broadcast :
		(msg["#msg-delivery"].get<std::string_view>() == "multicast" ? msg::delivery_t::multicast : msg::delivery_t::unicast) };

	if (!msg.contains("socket_id") or msg["socket_id"].get<std::string_view>().empty()) {
		std::shared_lock<decltype(chSubscribersLock)> lock(chSubscribersLock);
		for (auto&& [sess, subs] : chSubscribers) {
			if (sess != msg["#msg-from"].get<std::string_view>()) {
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
	else {
		std::shared_lock<decltype(chSubscribersLock)> lock(chSubscribersLock);
		if (auto&& subs{ chSubscribers.find(msg["socket_id"]) }; subs != chSubscribers.end()) {
			if (auto&& subsSelf{ subs->second.lock() }; subsSelf) {
				subsSelf->Push(message);
				++nSubscribers;
			}
			else {
				sessLeave.emplace_front(subs->first);
			}
		}
	}
	if (!sessLeave.empty()) {
		while (!sessLeave.empty()) {
			{
				std::unique_lock<decltype(chSubscribersLock)> lock(chSubscribersLock);
				chSubscribers.erase(sessLeave.front());
			}
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
