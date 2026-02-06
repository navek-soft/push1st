#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <deque>
#include <functional>
#include <mutex>
#include <thread>

class cratelimiter {
   public:
    explicit cratelimiter(size_t rps, size_t max_queue = 1000);
    ~cratelimiter();

    template <class Func>
    bool Enqueue(const Func& task);

   private:
    void Run(const std::stop_token& st);

   private:
    // Config
    const std::chrono::nanoseconds interval;
    const size_t max_queue;

    // Queue + worker
    std::mutex m_;
    std::condition_variable cv;
    std::deque<std::function<void(bool& skip)>> queue;
    std::chrono::steady_clock::time_point nextDue;
    std::jthread worker;
};

template <class Func>
bool cratelimiter::Enqueue(const Func& task) {
    {
        std::unique_lock lk(m_);
        if (queue.size() > max_queue) {
            return false;
        }
        queue.emplace_back(task);
    }
    cv.notify_one();
    return true;
}