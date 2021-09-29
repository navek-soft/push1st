#include "cchannel.h"
#include "../cmessage.h"
#include <forward_list>
#include "../csubscriber.h"
#include "../cchannels.h"

void cchannel::Push(std::unique_ptr<cmessage>&& msg) {
	std::forward_list<std::string> sessLeave;
	if (msg->To.empty()) {
		std::shared_lock<decltype(chSubscribersLock)> lock(chSubscribersLock);
		for (auto&& [sess, subs] : chSubscribers) {
			if (sess != msg->Producer) {
				if (auto&& subsSelf{ subs.lock() }; subsSelf) {
					subsSelf->Push(msg);
					if (msg->Delivery == delivery_t::broadcast) continue;
					break;
				}
				else {
					sessLeave.emplace_front(sess);
				}
			}
		}
	}
	else {
		for (auto&& sessId : msg->To)
		{
			if (sessId != msg->Producer) {
				std::shared_lock<decltype(chSubscribersLock)> lock(chSubscribersLock);
				if (auto&& subs{ chSubscribers.find(sessId) }; subs != chSubscribers.end()) {
					if (auto&& subsSelf{ subs->second.lock() }; subsSelf) {
						subsSelf->Push(msg);
						if (msg->Delivery == delivery_t::broadcast) continue;
						break;
					}
					else {
						sessLeave.emplace_front(subs->first);
					}
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
			sessLeave.pop_front();
		}

		std::unique_lock<decltype(chSubscribersLock)> lock(chSubscribersLock);
		if (chSubscribers.empty() and chMode == autoclose_t::yes) {
			chChannels->UnRegister(chUid);
		}
	}
}


cchannel::cchannel(const std::shared_ptr<cchannels>& channels, const std::string& cuid, const std::string& name, const app_t& app, channel_t type, autoclose_t mode) :
	chType{ type }, chMode{ mode }, chUid{ cuid }, chName{ name }, chApp{ app }, chChannels{ channels }
{
}

cchannel::~cchannel() {

}
