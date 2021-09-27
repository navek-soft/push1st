#include "csubscriber.h"
#include <atomic>
#include <chrono>
#include <random>

static std::atomic_uint64_t SessionId{ (uint64_t)std::chrono::system_clock::now().time_since_epoch().count() };

csubscriber::csubscriber(const std::string& ip, uint16_t port) 
{
	static char alphaRand[]{ "0123456789abcdefABCDEF" };
	uint64_t id{ ++SessionId };
	uint64_t sess{ std::hash<std::string>{}(ip) ^ (std::hash<uint64_t>{}(id) << 1) ^ (std::hash<uint16_t>{}(port) << 2) };
	subsId.assign(std::to_string(id / 1000000)).append(".");
	for (size_t n{ 7 }; n--;) { subsId.push_back(alphaRand[mrand48() % sizeof(alphaRand)]); }
	subsId.append(".").append(std::to_string(sess));
}

csubscriber::~csubscriber() {

}
