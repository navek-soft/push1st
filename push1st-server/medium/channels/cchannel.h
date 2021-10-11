#pragma once
#include "../medium.h"
#include "../ccredentials.h"
#include <shared_mutex>
#include <unordered_set>
#include "../cmessage.h"

enum class autoclose_t { yes = 1, no = 2};

class csubscriber;
class cchannels;


using usersids_t = std::unordered_set<std::string>;
using userslist_t = std::unordered_map<std::string, std::string>;

class cchannel {
public:
	cchannel(const std::shared_ptr<cchannels>& channels, const std::string& cuid, const std::string& name, const app_t& app, channel_t type, autoclose_t mode);
	virtual ~cchannel();
	virtual inline void Subscribe(const std::shared_ptr<csubscriber>& subscriber) = 0;
	virtual inline void UnSubscribe(const std::string& subscriber) = 0;
	virtual size_t Push(message_t&& msg);
	virtual inline void GetUsers(usersids_t&, userslist_t&) = 0;
	inline channel_t::type Type() { return chType; }
	inline size_t CountSubscribers() { return chSubscribers.size(); }
	virtual inline void OnClusterJoin(const json::value_t& val) { ; }
	virtual inline void OnClusterLeave(const json::value_t& val) { ; }
	virtual inline void OnClusterPush(message_t&& msg) { cchannel::Push(std::move(msg)); }
	virtual json::value_t ApiStats();
	virtual json::value_t ApiOverview();
protected:
	virtual inline void OnSubscriberLeave(const std::string& subscriber) = 0;
protected:
	channel_t chType{ channel_t::type::none };
	
	autoclose_t chMode{ autoclose_t::yes };
	std::string chUid, chName;
	app_t chApp;
	std::shared_ptr<cchannels> chChannels;
	std::shared_mutex chSubscribersLock;
	std::unordered_map<std::string, std::weak_ptr<csubscriber>> chSubscribers;
};

static inline channel_t ChannelType(const std::string_view& Name) {
	if (Name.compare(0, 8, "private-") == 0) { return channel_t::type::priv; }
	if (Name.compare(0, 9, "presense-") == 0) { return channel_t::type::pres; }
	if (Name.compare(0, 10, "protected-") == 0) { return channel_t::type::prot; }
	return channel_t::type::pub;
}