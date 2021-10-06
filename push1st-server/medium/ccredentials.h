#pragma once
#include "medium.h"
#include <unordered_map>
#include "../core/cconfig.h"

class cbroker;
class chook;

class ccredentials : public std::enable_shared_from_this<ccredentials>
{
	class capplication : public config::credential_t {
	public:
		capplication(const std::shared_ptr<cbroker>& broker, const config::credential_t& app);
		~capplication() = default;
		inline bool IsAllowChannel(channel_t::type type, const std::string& session, const std::string_view& channel, const std::string_view& token, const std::string& data = {}) {
			return (Channels & type) and ((type == channel_t::type::pub) or Validate(token, session, std::string{ channel }, data));
		}
		bool Validate(std::string_view token, const std::string& session, const std::string& channel, const std::string& custom_data);
		std::string Token(const std::string& session, const std::string& channel, const std::string& custom_data);
		void Trigger(hook_t::type type, sid_t channel, sid_t session, data_t);
	private:
		hook_t HookTriggers;
		std::vector<std::unique_ptr<chook>> HookEndpoints;
	};
public:
	using app_t = capplication;

	ccredentials(const std::shared_ptr<cbroker>& broker, const config::credentials_t& creds);
	~ccredentials();

	inline std::shared_ptr<app_t> GetAppByKey(const std::string& appKey) {
		if (auto&& it{ appCreds.find(appKey) }; it != appCreds.end()) {
			return it->second;
		}
		return {};
	}

	inline std::shared_ptr<app_t> GetAppById(const std::string& appId) {
		if (auto&& it{ appIds.find(appId) }; it != appIds.end()) {
			return it->second;
		}
		return {};
	}

private:
	std::unordered_map<std::string, std::shared_ptr<app_t>> appIds;
	std::unordered_map<std::string, std::shared_ptr<app_t>> appCreds;
};

using app_t = std::shared_ptr<ccredentials::app_t>;
using credentials_t = std::shared_ptr<ccredentials>;