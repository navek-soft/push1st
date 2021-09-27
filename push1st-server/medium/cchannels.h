#pragma once
#include "medium.h"
#include <shared_mutex>
#include <unordered_map>
#include "ccredentials.h"

class cchannel;

class cchannels : public std::enable_shared_from_this<cchannels>
{
public:
	cchannels();
	~cchannels();
	std::shared_ptr<cchannel> Register(channel_t type, const app_t& app, const std::string& name);
	void UnRegister(const std::string& name);
private:
	std::shared_mutex Sync;
	std::unordered_map<std::string, std::shared_ptr<cchannel>> Channels;
};

