#pragma once
#include "medium.h"
#include <shared_mutex>
#include "../core/ci/cspinlock.h"
#include <unordered_map>
#include "ccredentials.h"

class cchannel;

class cchannels : public std::enable_shared_from_this<cchannels>
{
public:
	cchannels();
	~cchannels();
	std::shared_ptr<cchannel> Register(channel_t type, const app_t& app, const std::string& name);
	inline std::shared_ptr<cchannel> Get(const app_t& app, const std::string& name) {
		std::string chUid{ app->Id + "#" + name };
		//std::shared_lock<decltype(Sync)> lock{ Sync };
		std::unique_lock<decltype(Sync)> lock{ Sync };
		if (auto&& ch{ Channels.find(chUid) }; ch != Channels.end()) {
			return ch->second;
		}
		return {};
	}
	void UnRegister(const std::string& name);
private:
	friend class cbroker;
	core::cspinlock Sync;
	//std::shared_mutex Sync;
	std::unordered_map<std::string, std::shared_ptr<cchannel>> Channels;
};

