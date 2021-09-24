#include "cwebsocket.h"
#include <random>
#include <ctime>
#include <openssl/sha.h>
#include "chttp.h"
#include <cstring>

using namespace http;

template<typename T>
static inline cwebsocket::msg_t make_un_frame(cwebsocket::opcode_t opcode, const std::string_view& message) {
	std::vector<uint8_t> payload(sizeof(T) + message.size());

	auto packet = new (payload.data()) T{ opcode,message.size() };
	std::memcpy(packet->payload, message.data(), message.size());

	return payload;
}

template<typename T>
static inline cwebsocket::msg_t make_frame(cwebsocket::opcode_t opcode, const std::string_view& message) {
	std::vector<uint8_t> payload(sizeof(T) + message.size());

	auto packet = new (payload.data()) T{ opcode,message.size() };
	cwebsocket::RandMask(packet->key);
	std::memcpy(packet->payload, message.data(), message.size());
	cwebsocket::Mask(packet->payload, message.size(), packet->key);

	return payload;
}

cwebsocket::msg_t cwebsocket::Make(cwebsocket::opcode_t opcode, const std::string_view& message, bool masked) {
	if (message.size() < 126) {
		if (!masked || message.size() == 0) {
			return make_un_frame<cwebsocket::small_un_frame_t>(opcode, message);
		}
		else {
			return make_frame<cwebsocket::small_frame_t>(opcode, message);
		}
	}
	else if (message.size() < 65536) {
		if (!masked) {
			return make_un_frame<cwebsocket::medium_un_frame_t>(opcode, message);
		}
		else {
			return make_frame<cwebsocket::medium_frame_t>(opcode, message);
		}
	}
	else {
		if (!masked) {
			return make_un_frame<cwebsocket::large_un_frame_t>(opcode, message);
		}
		else {
			return make_frame<cwebsocket::large_frame_t>(opcode, message);
		}
	}
	return { };
}

inline void cwebsocket::RandMask(mask_t& mask) {
	srand48(std::time(nullptr));
	*(uint32_t*)mask = (((uint32_t)lrand48()) << 1) ^ ((uint32_t)lrand48());
}

void cwebsocket::Mask(uint8_t* data, size_t length, const uint8_t* mask) {
	for (size_t n{ 0 }; n < length; ++n) { data[n] ^= mask[n % 4]; }
}

std::string cwebsocket::SecKey(const std::string_view& secKey) {
	if (SHA_CTX context; SHA1_Init(&context)) {
		uint8_t hash[SHA_DIGEST_LENGTH];
		SHA1_Update(&context, secKey.data(), secKey.size());
		SHA1_Update(&context, "258EAFA5-E914-47DA-95CA-C5AB0DC85B11", sizeof("258EAFA5-E914-47DA-95CA-C5AB0DC85B11") - 1);
		SHA1_Final(hash, &context);
		return http::ToBase64({ (const char*)hash, SHA_DIGEST_LENGTH });
	}
	return {};
}
