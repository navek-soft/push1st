#include "csubscriber.h"
#include <atomic>
#include <chrono>
#include <random>

static std::atomic_uint64_t SessionId{ (uint64_t)std::chrono::system_clock::now().time_since_epoch().count() };

csubscriber::csubscriber(const std::string& ip, uint16_t port) 
{
	uint64_t id{ ++SessionId };
	char sess[128];
	snprintf(sess, 127, "%lu.%010ld", id, std::rand());
	subsId.assign(sess);
}

csubscriber::~csubscriber() {

}
