#include "ccredentials.h"
#include "cbroker.h"
#include "chooks.h"
#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <openssl/sha.h>

static inline uint8_t hex(char h, char l) {
	if (h >= '0' and h <= '9') { h = (uint8_t)(h - '0'); } else if (h >= 'a' and h <= 'f') { h = (uint8_t)((h - 'a') + 10); } else return 0;
	if (l >= '0' and l <= '9') { l = (uint8_t)(l - '0'); } else if (l >= 'a' and l <= 'f') { l = (uint8_t)((l - 'a') + 10); } else return 0;
	return (uint8_t)((h << 4) + l);
}

static inline char hex(int v) {
	static const char alpha[]{ "0123456789abcdef" };
	return alpha[v];
}

std::string  ccredentials::capplication::Token(const std::string_view& session, const std::string& channel, const std::string& custom_data) {
	std::string data{ std::string{session} + ":" + channel + (custom_data.empty() ? "" : ":") + custom_data }, token;
	uint8_t digest[SHA256_DIGEST_LENGTH];
	uint dlength{ SHA256_DIGEST_LENGTH };
	token.reserve(dlength * 2);
	if (HMAC(EVP_sha256(), Secret.data(), (int)Secret.length(), (unsigned char*)data.data(), data.length(), digest, &dlength)) {
		for (uint n{ 0 }; n < dlength; ++n) {
			token.push_back(hex(digest[n] >> 4));
			token.push_back(hex(digest[n] & 0x0f));
		}
	}
	return token;
}

bool ccredentials::capplication::Validate(std::string_view token, const std::string_view& session, const std::string& channel, const std::string& custom_data) {
	std::string data{ std::string{session} + ":" + channel + (custom_data.empty() ? "" : ":") + custom_data };
	uint8_t digest[SHA256_DIGEST_LENGTH];
	uint dlength{ SHA256_DIGEST_LENGTH };
	if (token.compare(0, 4, "key:") == 0 and (token.length() - 4) == (SHA256_DIGEST_LENGTH * 2)) {
		if (token.remove_prefix(4); HMAC(EVP_sha256(), Secret.data(), (int)Secret.length(), (unsigned char*)data.data(), data.length(), digest, &dlength)) {
			for (uint n{ 0 }; n < dlength; ++n) {
				if (hex(token[0], token[1]) == digest[n]) { token.remove_prefix(2); continue; }
				return false;
			}
			return true;
		}
	}
	return false;
}
void ccredentials::capplication::Trigger(hook_t::type type, sid_t channel, sid_t session, data_t data) {
	for (auto&& [from, to] = HookEndpoints.equal_range(type); from != to; ++from) {
		if (from->second.first.Match(channel)) {
			from->second.second->Trigger(Id, channel, session, data);
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