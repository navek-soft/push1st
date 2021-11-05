#include "casyncqueue.h"

using namespace core;

void casyncqueue::threadRunner() {
	if (!quName.empty()) {
		pthread_setname_np(pthread_self(), quName.c_str());
	}

	while (quYet) {
		std::unique_lock<decltype(quLock)> lock(quLock);
		quNotify.wait(lock, [this]() { return !quJobs.empty() or !quYet; });

		if (quYet) {
			while (!quJobs.empty() and quYet)
			{
				auto job{ quJobs.front() };
				quJobs.pop();
				lock.unlock();
				job(); 
				lock.lock();
			}
			continue;
		}
		break;
	}
}

casyncqueue::casyncqueue(size_t poolsize, const std::string& name) : quYet{ true }, quName{ name } {
	std::unique_lock<decltype(quLock)> lock(quLock);
	quPool.reserve(poolsize);
	while (poolsize--) {
		quPool.emplace_back(std::bind(&casyncqueue::threadRunner, this));
	}
}

casyncqueue::~casyncqueue() {
	{
		std::unique_lock<decltype(quLock)> lock(quLock);
		quYet = false;
	}
	quNotify.notify_all();
	for (auto&& job : quPool) {
		if (job.joinable()) { job.join(); }
	}
}

