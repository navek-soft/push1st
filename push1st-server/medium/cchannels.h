#pragma once
#include "medium.h"
#include <shared_mutex>
#include "../core/ci/cspinlock.h"
#include <unordered_map>
#include "ccredentials.h"

class cchannel;
class ccluster;

class cchannels : public std::enable_shared_from_this<cchannels>
{
public:
	cchannels(const std::shared_ptr<ccluster>& cluster);
	~cchannels();
	std::shared_ptr<cchannel> Register(channel_t type, const app_t& app, const std::string& name);
	inline std::shared_ptr<cchannel> Get(const app_t& app, const std::string& name) {
		std::string chUid{ app->Id + "#" + name };
		std::unique_lock<decltype(Sync)> lock{ Sync };
		if (auto&& ch{ Channels.find(chUid) }; ch != Channels.end()) {
			return ch->second;
		}
		return {};
	}
	inline std::shared_ptr<cchannel> Get(const  std::string& app, const std::string& name) {
		std::string chUid{ app + "#" + name };
		std::unique_lock<decltype(Sync)> lock{ Sync };
		if (auto&& ch{ Channels.find(chUid) }; ch != Channels.end()) {
			return ch->second;
		}
		return {};
	}
	inline std::unordered_map<std::string, std::shared_ptr<cchannel>> List() {
		std::unique_lock<decltype(Sync)> lock{ Sync };
		return { Channels.begin(),Channels.end() };
	}
	void UnRegister(const std::string& name);
private:
	friend class cbroker;
	std::shared_ptr<ccluster> Cluster;
	core::cspinlock Sync;
	std::unordered_map<std::string, std::shared_ptr<cchannel>> Channels;
};

