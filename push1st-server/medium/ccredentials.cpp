#include "ccredentials.h"
#include "cbroker.h"
#include "chooks.h"
#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <openssl/sha.h>
#include "../core/csyslog.h"
#include "../core/ci/cbase62.h"

static ssize_t encrypt(const std::string& plaintext, const unsigned char* key, const unsigned char* iv, std::string& ciphertext);
static ssize_t decrypt(const std::string_view& ciphertext, const unsigned char* key, const unsigned char* iv, std::string& plaintext);

static inline uint8_t hex(char h, char l) {
	if (h >= '0' and h <= '9') { h = (uint8_t)(h - '0'); } else if (h >= 'a' and h <= 'f') { h = (uint8_t)((h - 'a') + 10); } else return 0;
	if (l >= '0' and l <= '9') { l = (uint8_t)(l - '0'); } else if (l >= 'a' and l <= 'f') { l = (uint8_t)((l - 'a') + 10); } else return 0;
	return (uint8_t)((h << 4) + l);
}

static inline char hex(int v) {
	static const char alpha[]{ "0123456789abcdef" };
	return alpha[v];
}

static inline void printhex(const char* name, std::string_view data) {
	printf("%s ", name);
	for (size_t n = 0; n < data.length(); ++n) {
		printf("%x", (uint8_t)data[n]);
	}
	printf("\n");
}


std::string ccredentials::capplication::AccessToken(const std::string& origin, const std::string& channel, size_t ttl) {

	try {
		size_t expire{ !ttl ? 0ul : (std::time(nullptr) + (std::time_t)ttl) };
		std::string token{ "token-" + std::to_string(std::rand()) + ":" + ( origin.empty() ? "*" : origin ) + ":" + (channel.empty() ? "*" : channel) + ":" + std::to_string(expire) };

		std::vector<uint8_t> key; key.resize(SHA256_DIGEST_LENGTH);
		uint klen{ (uint)key.size() };
		if (HMAC(EVP_sha256(), Secret.data(), (int)Secret.length(), (unsigned char*)AccessKeyIV.data(), AccessKeyIV.length(), key.data(), &klen)) {
			if (std::string encrypted; encrypt(token, key.data(), (uint8_t*)AccessKeyIV.data(), encrypted) == 0) {
				return encrypted;
			}
		}
	}
	catch (std::exception& ex) {

	}
	return {};
}

std::string  ccredentials::capplication::SessionToken(const std::string& session, const std::string& channel, const std::string& custom_data) {
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

static inline bool ValidateAccessToken(std::string_view token, const std::string_view& origin, const std::string_view& channel) {
	std::string_view args[4];
	for (auto npos{ token.find_first_of(':') }, n{ 0ul }; n < 4; ++n, npos = token.find_first_of(':')) {
		args[n] = token.substr(0, npos);
		token.remove_prefix(npos + 1);
		if (npos != std::string_view::npos) { continue; }
		else if (n != 3) { return false; }
	}
	try {
		std::time_t expire{ (std::time_t)std::stol(std::string{args[3]}) };
		return (
			(args[1] == "*" or origin == args[1]) and // check origin
			(args[2] == "*" or channel == args[2]) and // check channel
			(!expire or expire >= std::time(nullptr))); // check expire time
	}
	catch (std::exception& ex) {

	}
	return false;
}

static inline void Trim(std::string_view& data) {
	while (!data.empty() and std::isblank(data.front())) { data.remove_prefix(1); }
	while (!data.empty() and std::isblank(data.back())) { data.remove_suffix(1); }
}

bool ccredentials::capplication::ValidateSession(std::string_view token, const std::string& session, const std::string& channel, const std::string& custom_data, const std::string& origin) {
	
	if (token.compare(0, 4, "key:") == 0) {
		token.remove_prefix(4);
	}
	Trim(token);
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


bool ccredentials::capplication::ValidateToken(std::string_view token, const std::string& session, const std::string& channel, const std::string& custom_data, const std::string& origin) {
	if (strncasecmp(token.data(), "Bearer ", 7) == 0) {
		token.remove_prefix(7);
	}
	Trim(token);
	std::vector<uint8_t> key; key.resize(SHA256_DIGEST_LENGTH);
	uint klen{ (uint)key.size() };
	if (HMAC(EVP_sha256(), Secret.data(), (int)Secret.length(), (unsigned char*)AccessKeyIV.data(), AccessKeyIV.length(), key.data(), &klen)) {
		if (std::string decrypted; decrypt(token, key.data(), (uint8_t*)AccessKeyIV.data(), decrypted) == 0 and decrypted.compare(0, 6, "token-") == 0) {
			return ValidateAccessToken(decrypted, origin, channel);
		}
	}
	return false;
}

void ccredentials::capplication::Trigger(channel_t::type type, hook_t::type trigger, sid_t channel, sid_t session, json::value_t&& data) {
	if (Hooks & trigger and !HookEndpoints.empty()) {
		syslog.trace("[ HOOK:%s ] %s ( %s, %s ) \n", Id.c_str(), str(trigger), channel.c_str(), session.c_str());
		for (auto&& ep : HookEndpoints) { ep->Trigger(trigger, Id, channel, session, std::move(data)); }
	}
}

ccredentials::capplication::capplication(const std::shared_ptr<cbroker>& broker, const config::credential_t& app) :
	credential_t{ app } 
{
	syslog.ob.print("App", "%s ( %s ) ... %s", app.Id.c_str(), app.Name.c_str(), app.Enable ? "enable" : "disable");
	if (app.Enable) {
		for (auto&& ep : Endpoints) {
			HookEndpoints.emplace_back(broker->RegisterHook(ep, app.OptionKeepAlive));
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


ssize_t encrypt(const std::string& plaintext, const unsigned char* key, const unsigned char* iv, std::string& ciphertext)
{
	EVP_CIPHER_CTX* ctx;

	int len;

	int ciphertext_len;

	if (!(ctx = EVP_CIPHER_CTX_new()))
		return -1;

	std::vector<uint8_t> chipherdata;
	chipherdata.resize(plaintext.length() + 32);

	if (1 != EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv))
		return -1;

	if (1 != EVP_EncryptUpdate(ctx, chipherdata.data(), &len, (unsigned char*)plaintext.data(), (int)plaintext.size()))
		return -1;
	ciphertext_len = len;

	if (1 != EVP_EncryptFinal_ex(ctx, chipherdata.data() + len, &len))
		return -1;
	ciphertext_len += len;

	EVP_CIPHER_CTX_free(ctx);

	chipherdata.resize(ciphertext_len);

	ciphertext = ci::cbase62::encode(chipherdata.data(), chipherdata.size());

	return 0;
}

ssize_t decrypt(const  std::string_view& ciphertext, const unsigned char* key, const unsigned char* iv, std::string& plaintext)
{
	EVP_CIPHER_CTX* ctx;
	int len;

	int plaintext_len;

	if (!(ctx = EVP_CIPHER_CTX_new()))
		return -1;

	if (1 != EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv))
		return -1;

	std::vector<uint8_t> chipherdata{ ci::cbase62::decode((uint8_t*)ciphertext.data(),ciphertext.length()) };

	plaintext.resize(chipherdata.size() + 32);

	if (1 != EVP_DecryptUpdate(ctx, (uint8_t*)plaintext.data(), &len, chipherdata.data(), (int)chipherdata.size()))
		return -1;
	plaintext_len = len;

	if (1 != EVP_DecryptFinal_ex(ctx, (uint8_t*)plaintext.data() + len, &len))
		return -1;
	plaintext_len += len;

	EVP_CIPHER_CTX_free(ctx);

	plaintext.resize(plaintext_len);

	return 0;
}