#include "csubscriber.h"
#include <atomic>
#include <chrono>
#include <random>

static std::atomic_uint64_t SessionId{ (uint64_t)std::chrono::system_clock::now().time_since_epoch().count() };

csubscriber::csubscriber(const std::string& ip, uint16_t port) 
{
	uint64_t id{ ++SessionId };
	char sess[128];
	snprintf(sess, 127, "%lu%lu.%lu", id / std::chrono::system_clock::period::den, mrand48(),
		std::hash<std::string>{}(ip) ^ std::hash<uint64_t>{}(id) ^ (std::hash<uint16_t>{}(port) << 2) ^ __bswap_64(std::hash<uint64_t>{}(mrand48() ^ std::time(nullptr)))
	);
	subsId.assign(sess);
}

csubscriber::~csubscriber() {

}
