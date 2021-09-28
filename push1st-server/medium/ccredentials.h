#pragma once
#include "medium.h"
#include <unordered_map>
#include "../core/cconfig.h"

class cbroker;
class chook;

class ccredentials : public std::enable_shared_from_this<ccredentials>
{
	class capplication : public config::credential_t {
		class cmatch {
		public:
			cmatch() { ; }
			cmatch(const std::string& pattern);
			cmatch(const cmatch& ma) : Pattern{ ma.Pattern } { ; }
			~cmatch() = default;
			inline bool Match(const std::string& channel) { return Pattern.front() == '*' or Pattern.compare(0, Pattern.size(), channel) == 0; }
		private:
			std::string Pattern;
		};
	public:
		capplication(const std::shared_ptr<cbroker>& broker, const config::credential_t& app);
		~capplication() = default;
		inline bool IsAllowChannel(channel_t::type type, const std::string_view& session, const std::string_view& channel, const std::string_view& token, const std::string_view& data = {}) { return Channels & type; }
		bool Validate(std::string_view token, const std::string_view& session, const std::string& channel, const std::string& custom_data);
		std::string Token(const std::string_view& session, const std::string& channel, const std::string& custom_data);
		void Trigger(hook_t::type type, sid_t channel, sid_t session, data_t);
	private:
		std::unordered_multimap<hook_t::type, std::pair<cmatch /* channel matcher */, std::shared_ptr<chook> /* hook endpoint */>> HookEndpoints;
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