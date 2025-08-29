#pragma once
#include "medium.h"
#include <unordered_map>
#include "../core/cconfig.h"
#include "../core/ci/cjson.h"

class cbroker;
class chook;

class ccredentials : public std::enable_shared_from_this<ccredentials>
{
	class capplication : public config::credential_t {
	public:
		capplication(const std::shared_ptr<cbroker>& broker, const config::credential_t& app);
		~capplication() = default;
		inline bool IsAllowChannelSession(channel_t::type type, const std::string& session, const std::string_view& channel, const std::string_view& token, const std::string& data = {}, const std::string& origin = {}) {
			return (Channels & type) and (Origins.empty() or Origins.count(origin)) and  ((type == channel_t::type::pub ) or ValidateSession(token, session, std::string{ channel }, data, origin));
		}
		inline bool IsAllowChannelToken(channel_t::type type, const std::string& session, const std::string_view& channel, const std::string_view& token, const std::string& data = {}, const std::string& origin = {}) {
			return (Channels & type) and (Origins.empty() or Origins.count(origin)) and ((type == channel_t::type::pub) or ValidateToken(token, session, std::string{ channel }, data, origin));
		}
		inline bool IsAllowTrigger(channel_t::type type, hook_t::type trigger) {
			return (Channels & type) and (Hooks & trigger);
		}
		std::string SessionToken(const std::string& session, const std::string& channel, const std::string& custom_data);
		std::string AccessToken(const std::string& origin, const std::string& channel, size_t ttl);
		bool ValidateSession(std::string_view token, const std::string& session, const std::string& channel, const std::string& custom_data, const std::string& origin);
		bool ValidateToken(std::string_view token, const std::string& session, const std::string& channel, const std::string& custom_data, const std::string& origin);
		void Trigger(channel_t::type type, hook_t::type trigger, sid_t channel, sid_t session, json::value_t&&);
	private:
		std::vector<std::unique_ptr<chook>> HookEndpoints;
	};
public:
	using app_t = capplication;

	ccredentials(const std::shared_ptr<cbroker>& broker, const config::credentials_t& creds);
	~ccredentials();

	inline std::shared_ptr<app_t> GetAppByKey(const std::string& appKey) {
		if (auto&& it{ appCreds.find(appKey) }; it != appCreds.end() and it->second->Enable) {
			return it->second;
		}
		return {};
	}

	inline std::shared_ptr<app_t> GetAppById(const std::string& appId) {
		if (auto&& it{ appIds.find(appId) }; it != appIds.end() and it->second->Enable) {
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