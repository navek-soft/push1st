#include "cprivatechannel.h"
#include "../../core/csyslog.h"
#include "../csubscriber.h"
#include "../cchannels.h"
#include "../ccluster.h"

size_t cprivatechannel::Push(message_t&& msg) {
	Cluster->Push(chType, chApp, chName, *msg);
	return cchannel::Push(std::move(msg));
}

void cprivatechannel::Subscribe(const std::shared_ptr<csubscriber>& subscriber) {
	
	{
		std::unique_lock<decltype(chSubscribersLock)> lock(chSubscribersLock);
		chSubscribers.emplace(subscriber->Id(), std::dynamic_pointer_cast<csubscriber>(subscriber));
	}
	
	syslog.print(1, "[ PRIVATE:%s ] Subscribe %s ( %ld sessions)\n", chUid.c_str(), subscriber->Id().c_str(), chSubscribers.size());
	chApp->Trigger(chType, hook_t::type::join, chName, subscriber->Id(), {});
	Cluster->Trigger(chType, hook_t::type::join, chApp, chName, subscriber->Id(), {});
}

void cprivatechannel::OnSubscriberLeave(const std::string& subscriber) {
	syslog.print(1, "[ PRIVATE:%s ] UnSubscribe %s ( %ld sessions)\n", chUid.c_str(), subscriber.c_str(), chSubscribers.size());
	chApp->Trigger(chType, hook_t::type::leave, chName, subscriber, {});
	Cluster->Trigger(chType, hook_t::type::leave, chApp, chName, subscriber, {});
}

void cprivatechannel::UnSubscribe(const std::string& sessId) {
	{
		std::unique_lock<decltype(chSubscribersLock)> lock(chSubscribersLock, std::defer_lock);
		if (lock.try_lock()) {
			chSubscribers.erase(sessId);
			if (chSubscribers.empty() and chMode == autoclose_t::yes) {
				chChannels->UnRegister(chUid);
			}
		}
	}
	chApp->Trigger(chType, hook_t::type::leave, chName, sessId, {});
	Cluster->Trigger(chType, hook_t::type::leave, chApp, chName, sessId, {});
	syslog.print(1, "[ PRIVATE:%s ] UnSubscribe %s ( %ld sessions)\n", chUid.c_str(), sessId.c_str(), chSubscribers.size());
}

cprivatechannel::cprivatechannel(const std::shared_ptr<cchannels>& channels, const std::shared_ptr<ccluster>& cluster, const std::string& cuid, const std::string& name, const app_t& app, autoclose_t mode) :
	cchannel(channels, cuid, name, app, channel_t::type::priv, mode), Cluster{ cluster }
{

	syslog.print(1, "[ PRIVATE:%s ] Register\n", chUid.c_str());

	chApp->Trigger(chType, hook_t::type::reg, chName, {}, {});
	Cluster->Trigger(chType, hook_t::type::reg, chApp, chName, {}, {});
}

cprivatechannel::~cprivatechannel() {
	syslog.print(1, "[ PRIVATE:%s ] UnRegister\n", chUid.c_str());
	chApp->Trigger(chType, hook_t::type::unreg, chName, {}, {});
	Cluster->Trigger(chType, hook_t::type::unreg, chApp, chName, {}, {});
}