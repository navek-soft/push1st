#include "cpoll.h"
#include <unistd.h>
#include <sys/timerfd.h>

using namespace server;

void cpoll::OnPollTimer(int fd, uint) {
	uint64_t ts; ::read(fd, &ts, sizeof(ts));

	std::weak_ptr<cpipe> top;
	{
		std::unique_lock<decltype(pollGuard)> lock(pollGuard);
		if (!pollClients.empty()) {
			top = pollClients.front();
			pollClients.pop_front();
		}
	}
	if (auto&& cli{ top.lock() }; cli) {
		if (cli->OnCheck(std::time(nullptr))) {
			std::unique_lock<decltype(pollGuard)> lock(pollGuard);
			pollClients.push_back(cli);
		}
	}
}

void cpoll::PollThread(cpoll* self) {
	self->log_dbg("UP %lx\n", std::this_thread::get_id());

	pthread_setname_np(pthread_self(), "poll");

	if (epoll_ctl(self->pollFd, EPOLL_CTL_ADD, (int)(self->timerEvent), (epoll_event*)self->timerEvent) == 0) {
		ssize_t res{ 0 };
		std::vector<epoll_event> eventslist{ MaxEvents };
		do {
			if (res = epoll_wait(self->pollFd, eventslist.data(), (int)eventslist.size(), -1); res > 0) {
				while (res--) {
					((cevent*)eventslist[res].data.ptr)->trigger(eventslist[res].events, self->shared_from_this());
				}
				continue;
			}

			if (auto err = errno; err == EINTR) {
				self->log_dbg("ERR %lx ( %s )\n", std::this_thread::get_id(), std::strerror(err));
				continue;
			}
			else {
				self->err("ERR %lx ( %s )\n", std::this_thread::get_id(), std::strerror(err));
			}
			break;
		} while (1);
	}
	self->log_dbg("DOWN %lx\n", std::this_thread::get_id());
}

void cpoll::Join() {
#ifndef DEBUG
	if (pollFd > 0) {
		::close(pollFd);
		pollFd = -1;
	}
#endif // !DEBUG
	if (pollThread.joinable()) { pollThread.join(); }
}

cpoll::cpoll(std::time_t timerMilliseconds) :
	csyslog{ "[ POLL ]" },
	timerEvent{ std::bind(&cpoll::OnPollTimer,this,std::placeholders::_1,std::placeholders::_2) }
{
	if (pollFd = epoll_create1(EPOLL_CLOEXEC); pollFd > 0) {

		struct itimerspec timer { {.tv_sec = (timerMilliseconds - (timerMilliseconds % 1000)) / 1000, .tv_nsec = (timerMilliseconds % 1000) * 1000000 },
			{ .tv_sec = (timerMilliseconds - (timerMilliseconds % 1000)) / 1000, .tv_nsec = (timerMilliseconds % 1000) * 1000000 }
		};

		if (int fd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC); fd > 0 and timerfd_settime(fd, TFD_TIMER_ABSTIME, &timer, nullptr) == 0) {
			timerEvent(fd, EPOLLIN | EPOLLET);
			pollThread = std::thread(&cpoll::PollThread, this);
			return;
		}
		throw std::runtime_error(std::string{ "TIMER Initialize error ( " }.append(strerror(errno)).append(" )"));
	}
	throw std::runtime_error(std::string{ "INIT Create error ( " }.append(strerror(errno)).append(" )"));
}

cpoll::~cpoll() {
	Join();
}