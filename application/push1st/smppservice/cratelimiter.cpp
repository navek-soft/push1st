#include "cratelimiter.h"

#include <mutex>

void cratelimiter::Run(const std::stop_token& st) {
    for (;;) {
        std::unique_lock<std::mutex> lk(m_);
        cv.wait(lk, [&] {
            return st.stop_requested() or not queue.empty();
        });
        if (st.stop_requested()) {
            break;
        }

        while (not queue.empty() and not st.stop_requested()) {
            auto job = queue.front();
            queue.pop_front();
            lk.unlock();

            auto now = std::chrono::steady_clock::now();
            if (now < nextDue) {
                std::this_thread::sleep_until(nextDue);
                now = nextDue;
            }
            bool skip {false};
            if (job(skip); skip) {
                nextDue = now;
            } else {
                nextDue = (nextDue < now) ? now + interval : nextDue + interval;
            }

            lk.lock();
        }
    }
}

cratelimiter::cratelimiter(size_t rps, size_t max_queue) :
    interval {std::chrono::nanoseconds(1'000'000'000 / (rps ? rps : 1))},
    max_queue {max_queue},
    nextDue {std::chrono::steady_clock::now()},
    worker([this](const std::stop_token& st) {
        Run(st);
    }) {}

cratelimiter::~cratelimiter() {
    worker.request_stop();
    cv.notify_all();
    if (worker.joinable()) {
        worker.join();
    }
}
