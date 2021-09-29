#include "cpublicchannel.h"
#include "../../core/csyslog.h"
#include "../csubscriber.h"
#include "../cchannels.h"

void cpublicchannel::Subscribe(const std::shared_ptr<csubscriber>& subscriber) {
	std::unique_lock<decltype(chSubscribersLock)> lock(chSubscribersLock);
	chSubscribers.emplace(subscriber->Id(), std::dynamic_pointer_cast<csubscriber>(subscriber));

	syslog.print(1, "[ PUBLIC:%s ] Subscribe %s ( %ld sessions)\n", chUid.c_str(), subscriber->Id().c_str(), chSubscribers.size());

	chApp->Trigger(hook_t::type::join, chName, subscriber->Id(), {});
}

void cpublicchannel::UnSubscribe(const std::shared_ptr<csubscriber>& subscriber) {
	std::unique_lock<decltype(chSubscribersLock)> lock(chSubscribersLock);
	chSubscribers.erase(subscriber->Id());

	syslog.print(1, "[ PUBLIC:%s ] UnSubscribe %s ( %ld sessions)\n", chUid.c_str(), subscriber->Id().c_str(), chSubscribers.size());

	chApp->Trigger(hook_t::type::leave, chName, subscriber->Id(), {});

	if (chSubscribers.empty() and chMode == autoclose_t::yes) {
		chChannels->UnRegister(chUid);
	}
}

cpublicchannel::cpublicchannel(const std::shared_ptr<cchannels>& channels, const std::string& cuid, const std::string& name, const app_t& app, autoclose_t mode) :
	cchannel(channels, cuid, name, app, channel_t::type::pub, mode)
{

	syslog.print(1, "[ PUBLIC:%s ] Register\n", chUid.c_str());

	chApp->Trigger(hook_t::type::reg, chName, {}, {});
}

cpublicchannel::~cpublicchannel() { 
	syslog.print(1, "[ PUBLIC:%s ] UnRegister\n", chUid.c_str());
	chApp->Trigger(hook_t::type::unreg, chName, {}, {});
}