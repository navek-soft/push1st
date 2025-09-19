#pragma once
#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <vector>

namespace core {
class casyncqueue {
   public:
    casyncqueue(size_t poolsize, const std::string& name = {});
    ~casyncqueue();
    inline void Enqueue(const std::function<void()>& job);

   private:
    void ThreadRunner();

   private:
    volatile bool quYet {false};
    std::string quName;
    std::mutex quLock;
    std::condition_variable quNotify;
    std::vector<std::thread> quPool;
    std::queue<std::function<void()>> quJobs;
};
inline void casyncqueue::Enqueue(const std::function<void()>& job) {
    {
        std::unique_lock<decltype(quLock)> lock(quLock);
        quJobs.emplace(job);
    }
    quNotify.notify_one();
}
}// namespace core
