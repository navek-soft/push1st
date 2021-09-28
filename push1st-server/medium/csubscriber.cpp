#include "csubscriber.h"
#include <atomic>
#include <chrono>
#include <random>

static std::atomic_uint64_t SessionId{ (uint64_t)std::chrono::system_clock::now().time_since_epoch().count() };

csubscriber::csubscriber(const std::string& ip, uint16_t port) 
{
	static char alphaRand[]{ "0123456789abcdefABCDEF" };
	uint64_t id{ ++SessionId };
	char sess[128];
	snprintf(sess, 127, "%lX.%c%c%c%c%c%c%c.%lX", id,
		alphaRand[mrand48() % sizeof(alphaRand)], alphaRand[mrand48() % sizeof(alphaRand)], alphaRand[mrand48() % sizeof(alphaRand)],
		alphaRand[mrand48() % sizeof(alphaRand)], alphaRand[mrand48() % sizeof(alphaRand)], alphaRand[mrand48() % sizeof(alphaRand)], alphaRand[mrand48() % sizeof(alphaRand)],
		std::hash<std::string>{}(ip) ^ std::hash<uint64_t>{}(id) ^ (std::hash<uint16_t>{}(port) << 2) ^ std::hash<uint64_t>{}(std::time(nullptr))
	);
	subsId.assign(sess);
}

csubscriber::~csubscriber() {

}
