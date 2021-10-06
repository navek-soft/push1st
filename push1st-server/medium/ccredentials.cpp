#include "ccredentials.h"
#include "cbroker.h"
#include "chooks.h"
#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <openssl/sha.h>
#include "../core/csyslog.h"

static inline uint8_t hex(char h, char l) {
	if (h >= '0' and h <= '9') { h = (uint8_t)(h - '0'); } else if (h >= 'a' and h <= 'f') { h = (uint8_t)((h - 'a') + 10); } else return 0;
	if (l >= '0' and l <= '9') { l = (uint8_t)(l - '0'); } else if (l >= 'a' and l <= 'f') { l = (uint8_t)((l - 'a') + 10); } else return 0;
	return (uint8_t)((h << 4) + l);
}

static inline char hex(int v) {
	static const char alpha[]{ "0123456789abcdef" };
	return alpha[v];
}

std::string  ccredentials::capplication::Token(const std::string& session, const std::string& channel, const std::string& custom_data) {
	std::string data{ session + ":" + channel + (custom_data.empty() ? "" : ":") + custom_data }, token;
	std::vector<uint8_t> digest; digest.resize(SHA256_DIGEST_LENGTH);
	uint dlength{ (uint)digest.size() };
	token.reserve(dlength * 2);
	if (HMAC(EVP_sha256(), Secret.data(), (int)Secret.length(), (unsigned char*)data.data(), data.length(), digest.data(), &dlength)) {
		for (uint n{ 0 }; n < dlength; ++n) {
			token.push_back(hex(digest[n] >> 4));
			token.push_back(hex(digest[n] & 0x0f));
		}
	}
	return token;
}

bool ccredentials::capplication::Validate(std::string_view token, const std::string& session, const std::string& channel, const std::string& custom_data) {
	if (token.compare(0, 4, "key:") == 0) {
		token.remove_prefix(4);
	}
	if (token.length() >= (SHA256_DIGEST_LENGTH * 2)) {
		std::string data{ std::string{session} + ":" + channel + (custom_data.empty() ? "" : ":") + custom_data };
		std::vector<uint8_t> digest; digest.resize(SHA256_DIGEST_LENGTH);
		uint dlength{ (uint)digest.size() };
		if (HMAC(EVP_sha256(), Secret.data(), (int)Secret.length(), (unsigned char*)data.data(), data.length(), digest.data(), &dlength)) {
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
	if (Hooks & type and !HookEndpoints.empty()) {
		syslog.trace("[ HOOK:%s ] %s ( %s, %s ) \n", Id.c_str(), str(type), channel.c_str(), session.c_str());
		for (auto&& ep : HookEndpoints) { ep->Trigger(type, Id, channel, session, data); } 
	}
}

ccredentials::capplication::capplication(const std::shared_ptr<cbroker>& broker, const config::credential_t& app) :
	credential_t{ app } 
{
	for (auto&& ep : Endpoints) {
		HookEndpoints.emplace_back(broker->RegisterHook(ep));
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