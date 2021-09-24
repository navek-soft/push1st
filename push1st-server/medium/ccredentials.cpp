#include "ccredentials.h"
#include "cbroker.h"
#include "chooks.h"

void ccredentials::capplication::Trigger(hook_t::type type, sid_t channel, sid_t session, data_t data) {
	for (auto&& [from, to] = HookEndpoints.equal_range(type); from != to; ++from) {
		if (from->second.first.Match(channel)) {
			from->second.second->Trigger(channel, session, data);
		}
	}
}

ccredentials::capplication::cmatch::cmatch(const std::string& pattern) {
	if (pattern == "*") {
		Pattern = pattern;
	}
	else if (auto pos{ pattern.find_first_of('*') }; pos != std::string::npos) {
		Pattern = pattern.substr(0, pos);
	}
	else {
		Pattern = pattern;
	}
}

ccredentials::capplication::capplication(const std::shared_ptr<cbroker>& broker, const config::credential_t& app) :
	credential_t{ app } 
{
	for (auto&& [tp, hook] : Hooks) {
		if (!hook.first.empty() and !hook.second.empty()) {
			HookEndpoints.emplace(tp, std::make_pair(hook.first, broker->RegisterHook(hook.second)));
		}
	}
}

ccredentials::ccredentials(const std::shared_ptr<cbroker>& broker, const config::credentials_t& creds) {
	for (auto& app : creds.Apps) {
		if (app.Id.empty() or app.Channels.empty()) continue;
		auto&& ptr{ std::make_shared<ccredentials::app_t>(broker, app) };
		appCreds.emplace(ptr->Key, ptr);
		appIds.emplace(ptr->Id, ptr);
	}
}

ccredentials::~ccredentials() {

}