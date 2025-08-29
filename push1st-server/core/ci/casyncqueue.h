#pragma once
#include <thread>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <queue>
#include <functional>
#include <string>

namespace core {
	class casyncqueue {
	public:
		casyncqueue(size_t poolsize, const std::string& name = {});
		~casyncqueue();
		inline void enqueue(const std::function<void()>& job);
	private:
		void threadRunner();
	private:
		volatile bool quYet{ false };
		std::string quName;
		std::mutex quLock;
		std::condition_variable quNotify;
		std::vector<std::thread> quPool;
		std::queue<std::function<void()>> quJobs;
	};
	inline void casyncqueue::enqueue(const std::function<void()>& job) {
		{
			std::unique_lock<decltype(quLock)> lock(quLock);
			quJobs.emplace(job);
		}
		quNotify.notify_one();
	}
}
