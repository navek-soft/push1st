#pragma once
#include "../medium.h"
#include "../ccredentials.h"
#include <shared_mutex>

enum class autoclose_t { yes = 1, no = 2};

class csubscriber;
class cchannels;
class cmessage;

class cchannel {
public:
	cchannel(const std::shared_ptr<cchannels>& channels, const std::string& cuid, const std::string& name, const app_t& app, channel_t type, autoclose_t mode);
	virtual ~cchannel();
	virtual inline void Subscribe(const std::shared_ptr<csubscriber>& subscriber) = 0;
	virtual inline void UnSubscribe(const std::shared_ptr<csubscriber>& subscriber) = 0;
	virtual inline void Push(std::unique_ptr<cmessage>&& msg) = 0;
protected:
	channel_t chType{ channel_t::type::none };
	autoclose_t chMode{ autoclose_t::yes };
	std::string chUid, chName;
	app_t chApp;
	std::shared_ptr<cchannels> chChannels;
	std::shared_mutex chSubscribersLock;
};

static inline channel_t ChannelType(const std::string_view& Name) {
	if (Name.compare(0, 8, "private-") == 0) { return channel_t::type::priv; }
	if (Name.compare(0, 9, "presense-") == 0) { return channel_t::type::pres; }
	if (Name.compare(0, 10, "protected-") == 0) { return channel_t::type::prot; }
	return channel_t::type::pub;
}