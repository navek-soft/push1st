#include "core/credentials/ccredentials.h"

#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <openssl/sha.h>

#include "broker/api.h"
#include "core/hooks/chooks.h"
#include "core/util/cbase62.h"
#include "log/clog.h"

static ssize_t Encrypt(const std::string& plaintext, const unsigned char* key, const unsigned char* iv, std::string& ciphertext);
static ssize_t Decrypt(const std::string_view& ciphertext, const unsigned char* key, const unsigned char* iv, std::string& plaintext);

static inline uint8_t Hex(char h, char l) {
    if (h >= '0' and h <= '9') {
        h = (uint8_t)(h - '0');
    } else if (h >= 'a' and h <= 'f') {
        h = (uint8_t)((h - 'a') + 10);
    } else {
        return 0;
    }
    if (l >= '0' and l <= '9') {
        l = (uint8_t)(l - '0');
    } else if (l >= 'a' and l <= 'f') {
        l = (uint8_t)((l - 'a') + 10);
    } else {
        return 0;
    }
    return (uint8_t)((h << 4) + l);
}

static inline char Hex(int v) {
    static const char alpha[] {"0123456789abcdef"};
    return alpha[v];
}

std::string ccredentials::capplication::AccessToken(const std::string& origin, const std::string& channel, size_t ttl) {
    try {
        size_t expire {!ttl ? 0UL : (std::time(nullptr) + (std::time_t)ttl)};
        std::string token {"token-" + std::to_string(std::rand()) + ":" + (origin.empty() ? "*" : origin) + ":" + (channel.empty() ? "*" : channel) + ":" + std::to_string(expire)};

        std::vector<uint8_t> key;
        key.resize(SHA256_DIGEST_LENGTH);
        uint klen {(uint)key.size()};
        if (HMAC(EVP_sha256(), Secret.data(), (int)Secret.length(), (unsigned char*)AccessKeyIV.data(), AccessKeyIV.length(), key.data(), &klen)) {
            if (std::string encrypted; Encrypt(token, key.data(), (uint8_t*)AccessKeyIV.data(), encrypted) == 0) {
                return encrypted;
            }
        }
    } catch (std::exception& ex) {}
    return {};
}

std::string ccredentials::capplication::SessionToken(const std::string& session, const std::string& channel, const std::string& custom_data) {
    std::string data {session + ":" + channel + (custom_data.empty() ? "" : ":") + custom_data}, token;
    std::vector<uint8_t> digest;
    digest.resize(SHA256_DIGEST_LENGTH);
    uint dlength {(uint)digest.size()};
    token.reserve(dlength * 2);
    if (HMAC(EVP_sha256(), Secret.data(), (int)Secret.length(), (unsigned char*)data.data(), data.length(), digest.data(), &dlength)) {
        for (uint n {0}; n < dlength; ++n) {
            token.push_back(Hex(digest[n] >> 4));
            token.push_back(Hex(digest[n] & 0x0f));
        }
    }
    return token;
}

static inline bool ValidateAccessToken(std::string_view token, const std::string_view& origin, const std::string_view& channel) {
    std::string_view args[4];
    for (auto npos {token.find_first_of(':')}, n {0UL}; n < 4; ++n, npos = token.find_first_of(':')) {
        args[n] = token.substr(0, npos);
        token.remove_prefix(npos + 1);
        if (npos != std::string_view::npos) {
            continue;
        } else if (n != 3) {
            return false;
        }
    }
    try {
        std::time_t expire {(std::time_t)std::stol(std::string {args[3]})};
        return ((args[1] == "*" or origin == args[1]) and  // check origin
                (args[2] == "*" or channel == args[2]) and // check channel
                (!expire or expire >= std::time(nullptr)));// check expire time
    } catch (std::exception& ex) {}
    return false;
}

static inline void Trim(std::string_view& data) {
    while (!data.empty() and std::isblank(data.front())) {
        data.remove_prefix(1);
    }
    while (!data.empty() and std::isblank(data.back())) {
        data.remove_suffix(1);
    }
}

bool ccredentials::capplication::ValidateSession(std::string_view token, const std::string& session, const std::string& channel, const std::string& custom_data, [[maybe_unused]] const std::string& origin) {
    if (auto npos {token.find_first_of(':')}; npos != std::string_view::npos) {
        token.remove_prefix(npos + 1);
    }
    Trim(token);
    if (token.length() >= (SHA256_DIGEST_LENGTH * 2)) {
        std::string data {std::string {session} + ":" + channel + (custom_data.empty() ? "" : ":") + custom_data};
        std::vector<uint8_t> digest;
        digest.resize(SHA256_DIGEST_LENGTH);
        uint dlength {(uint)digest.size()};
        if (HMAC(EVP_sha256(), Secret.data(), (int)Secret.length(), (unsigned char*)data.data(), data.length(), digest.data(), &dlength)) {
            for (uint n {0}; n < dlength; ++n) {
                if (Hex(token[0], token[1]) == digest[n]) {
                    token.remove_prefix(2);
                    continue;
                }
                return false;
            }
            return true;
        }
    }
    return false;
}

bool ccredentials::capplication::ValidateToken(std::string_view token, [[maybe_unused]] const std::string& session, const std::string& channel, [[maybe_unused]] const std::string& custom_data, const std::string& origin) {
    if (token.size() >= 7 and strncasecmp(token.data(), "Bearer ", 7) == 0) {
        token.remove_prefix(7);
    }
    Trim(token);
    std::vector<uint8_t> key;
    key.resize(SHA256_DIGEST_LENGTH);
    uint klen {(uint)key.size()};
    if (HMAC(EVP_sha256(), Secret.data(), (int)Secret.length(), (unsigned char*)AccessKeyIV.data(), AccessKeyIV.length(), key.data(), &klen)) {
        if (std::string decrypted;
            Decrypt(token, key.data(), (uint8_t*)AccessKeyIV.data(), decrypted) == 0 and decrypted.compare(0, 6, "token-") == 0) {
            return ValidateAccessToken(decrypted, origin, channel);
        }
    }
    return false;
}

void ccredentials::capplication::Trigger([[maybe_unused]] channel_t::type type, hook_t::type trigger, sid_t channel, sid_t session, json::value_t&& data) {
    if (Hooks & trigger and !HookEndpoints.empty()) {
        PSHT_INFO("HOOK:{} {} ( {}, {} )", Id.c_str(), Str(trigger), channel.c_str(), session.c_str());
        for (auto&& ep : HookEndpoints) {
            ep->Trigger(trigger, Id, channel, session, std::move(data));// NOLINT (need fix)
        }
    }
}

ccredentials::capplication::capplication(const broker_t& broker, const config::credential_t& app) : credential_t {app} {
    PSHT_INFO("App {} ( {} ) ... {}", app.Id.c_str(), app.Name.c_str(), app.Enable ? "enable" : "disable");
    if (app.Enable) {
        for (auto&& ep : Endpoints) {
            HookEndpoints.emplace_back(broker->RegisterHook(ep, app.OptionKeepAlive));
        }
    }
}

ccredentials::ccredentials(const broker_t& broker, config::credentials_t& creds) {
    for (auto& app : creds.Apps) {
        if (app.Id.empty() or app.Channels.Empty()) {
            continue;
        }
        auto&& ptr {std::make_shared<ccredentials::app_t>(broker, app)};
        appCreds.emplace(ptr->Key, ptr);
        appIds.emplace(ptr->Id, ptr);
    }
}

ccredentials::~ccredentials() {}

ssize_t Encrypt(const std::string& plaintext, const unsigned char* key, const unsigned char* iv, std::string& ciphertext) {
    EVP_CIPHER_CTX* ctx;

    int len;

    int ciphertext_len;

    if (!(ctx = EVP_CIPHER_CTX_new())) {
        return -1;
    }

    std::vector<uint8_t> chipherdata;
    chipherdata.resize(plaintext.length() + 32);

    if (1 != EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr, key, iv)) {
        return -1;
    }

    if (1 != EVP_EncryptUpdate(ctx, chipherdata.data(), &len, (unsigned char*)plaintext.data(), (int)plaintext.size())) {
        return -1;
    }
    ciphertext_len = len;

    if (1 != EVP_EncryptFinal_ex(ctx, chipherdata.data() + len, &len)) {
        return -1;
    }
    ciphertext_len += len;

    EVP_CIPHER_CTX_free(ctx);

    chipherdata.resize(ciphertext_len);

    ciphertext = ci::cbase62::Encode(chipherdata.data(), chipherdata.size());

    return 0;
}

ssize_t Decrypt(const std::string_view& ciphertext, const unsigned char* key, const unsigned char* iv, std::string& plaintext) {
    EVP_CIPHER_CTX* ctx;
    int len;

    int plaintext_len;

    if (!(ctx = EVP_CIPHER_CTX_new())) {
        return -1;
    }

    if (1 != EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr, key, iv)) {
        return -1;
    }

    std::vector<uint8_t> chipherdata {ci::cbase62::Decode((uint8_t*)ciphertext.data(), ciphertext.length())};

    plaintext.resize(chipherdata.size() + 32);

    if (1 != EVP_DecryptUpdate(ctx, (uint8_t*)plaintext.data(), &len, chipherdata.data(), (int)chipherdata.size())) {
        return -1;
    }
    plaintext_len = len;

    if (1 != EVP_DecryptFinal_ex(ctx, (uint8_t*)plaintext.data() + len, &len)) {
        return -1;
    }
    plaintext_len += len;

    EVP_CIPHER_CTX_free(ctx);

    plaintext.resize(plaintext_len);

    return 0;
}