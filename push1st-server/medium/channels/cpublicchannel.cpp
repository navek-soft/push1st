#include "cpublicchannel.h"
#include "../../core/csyslog.h"
#include "../csubscriber.h"
#include "../cchannels.h"
#include "../cmessage.h"
#include <forward_list>

void cpublicchannel::Push(std::unique_ptr<cmessage>&& msg) {
	std::forward_list<std::string> sessLeave;
	if (msg->To.empty()) {
		std::shared_lock<decltype(chSubscribersLock)> lock(chSubscribersLock);
		for (auto&& [sess, subs] : chSubscribers) {
			if (sess != msg->Producer) {
				if (auto&& subsSelf{ subs.lock() }; subsSelf) {
					subsSelf->Push(msg);
					if (msg->Delivery != delivery_t::broadcast) continue;
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
						if (msg->Delivery != delivery_t::broadcast) continue;
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

void cpublicchannel::Subscribe(const std::shared_ptr<csubscriber>& subscriber) {
	std::unique_lock<decltype(chSubscribersLock)> lock(chSubscribersLock);
	chSubscribers.emplace(subscriber->Id(), std::dynamic_pointer_cast<csubscriber>(subscriber));

	syslog.print(1, "[CHANNEL:%s] Subscribe %s ( %ld sessions)\n", chUid.c_str(), subscriber->Id().c_str(), chSubscribers.size());

	chApp->Trigger(hook_t::type::join, chName, subscriber->Id(), {});
}

void cpublicchannel::UnSubscribe(const std::shared_ptr<csubscriber>& subscriber) {
	std::unique_lock<decltype(chSubscribersLock)> lock(chSubscribersLock);
	chSubscribers.erase(subscriber->Id());

	syslog.print(1, "[CHANNEL:%s] UnSubscribe %s ( %ld sessions)\n", chUid.c_str(), subscriber->Id().c_str(), chSubscribers.size());

	chApp->Trigger(hook_t::type::leave, chName, subscriber->Id(), {});

	if (chSubscribers.empty() and chMode == autoclose_t::yes) {
		chChannels->UnRegister(chUid);
	}
}

cpublicchannel::cpublicchannel(const std::shared_ptr<cchannels>& channels, const std::string& cuid, const std::string& name, const app_t& app, autoclose_t mode) :
	cchannel(channels, cuid, name, app, channel_t::type::pub, mode)
{

	syslog.print(1, "[CHANNEL:%s] Register\n", chUid.c_str());

	chApp->Trigger(hook_t::type::reg, chName, {}, {});
}

cpublicchannel::~cpublicchannel() { 
	syslog.print(1, "[CHANNEL:%s] UnRegister\n", chUid.c_str());
	chApp->Trigger(hook_t::type::unreg, chName, {}, {});
}