#include "cprivatechannel.h"
#include "../../core/csyslog.h"
#include "../csubscriber.h"
#include "../cchannels.h"

void cprivatechannel::Subscribe(const std::shared_ptr<csubscriber>& subscriber) {
	std::unique_lock<decltype(chSubscribersLock)> lock(chSubscribersLock);
	chSubscribers.emplace(subscriber->Id(), std::dynamic_pointer_cast<csubscriber>(subscriber));

	syslog.print(1, "[ PRIVATE:%s ] Subscribe %s ( %ld sessions)\n", chUid.c_str(), subscriber->Id().c_str(), chSubscribers.size());

	chApp->Trigger(hook_t::type::join, chName, subscriber->Id(), {});
}

void cprivatechannel::UnSubscribe(const std::string& sessId) {
	std::unique_lock<decltype(chSubscribersLock)> lock(chSubscribersLock);
	syslog.print(1, "[ PUBLIC:%s ] UnSubscribe %s ( %ld sessions)\n", chUid.c_str(), sessId.c_str(), chSubscribers.size());

	chSubscribers.erase(sessId);

	chApp->Trigger(hook_t::type::leave, chName, sessId, {});

	if (chSubscribers.empty() and chMode == autoclose_t::yes) {
		chChannels->UnRegister(chUid);
	}
}

cprivatechannel::cprivatechannel(const std::shared_ptr<cchannels>& channels, const std::string& cuid, const std::string& name, const app_t& app, autoclose_t mode) :
	cchannel(channels, cuid, name, app, channel_t::type::pub, mode)
{

	syslog.print(1, "[ PRIVATE:%s ] Register\n", chUid.c_str());

	chApp->Trigger(hook_t::type::reg, chName, {}, {});
}

cprivatechannel::~cprivatechannel() {
	syslog.print(1, "[ PRIVATE:%s ] UnRegister\n", chUid.c_str());
	chApp->Trigger(hook_t::type::unreg, chName, {}, {});
}