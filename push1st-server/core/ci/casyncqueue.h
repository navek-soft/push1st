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
		template<typename FN, typename ... ARGS>
		void enqueue(FN job, ARGS&& ... args);
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
	template<typename FN, typename ... ARGS>
	void casyncqueue::enqueue(FN job, ARGS&& ... args) {
		{
			std::unique_lock<decltype(quLock)> lock(quLock);
			std::tuple<ARGS...> jobargs(args...);
			quJobs.emplace([job, jobargs]() {std::apply(job, jobargs); });
		}
		quNotify.notify_one();
	}
}
