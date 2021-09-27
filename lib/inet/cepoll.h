#pragma once
#include "cinet.h"
#include <sys/epoll.h>
#include <thread>
#include <unordered_map>
#include <functional>

namespace inet {
	class cpoll : public std::enable_shared_from_this<cpoll> {
	public:
		cpoll();
		virtual ~cpoll();
		virtual const char* NameOf() { return "epoll"; }
		ssize_t Listen(int pollSize = 1024, int pollTimeoutMs = -1);
		void Join();
	protected:
		inline std::shared_ptr<cpoll> Self() { return shared_from_this(); }
	public:
		template<typename FN>
		inline ssize_t PollAdd(fd_t fd, uint events, FN&& callback);
		template<typename FN>
		inline ssize_t PollUpdate(fd_t fd, FN&& callback);
		template<typename FN>
		inline ssize_t PollUpdate(fd_t fd, uint events, FN&& callback);
		ssize_t PollUpdate(fd_t fd, uint events);
		void PollDelete(int& fd);
		inline fd_t Fd() { return fdPoll; }
	private:
		static void PollThread(std::shared_ptr<cpoll> self, int numEventsMax, int msTimeout);
		using handler_t = std::function<void(fd_t,uint)>;
		fd_t fdPoll{ -1 };
		std::unordered_map<fd_t, handler_t> fdHandlers;
		std::thread pollThread;
	};
	template<typename FN_T>
	inline ssize_t cpoll::PollAdd(fd_t fd, uint events, FN_T&& callback) {
		if (struct epoll_event ev { .events = events, .data = { .fd = fd } }; epoll_ctl(fdPoll, EPOLL_CTL_ADD, fd, &ev) == 0) {
			fdHandlers[fd] = std::move(callback);
			return 0;
		}
		return -errno;
	}
	template<class FN_T>
	inline ssize_t cpoll::PollUpdate(fd_t fd, FN_T&& callback) {
		if (auto&& it{ fdHandlers.find(fd) }; it != fdHandlers.end()) {
			fdHandlers[fd] = std::move(callback);
			return 0;
		}
		return -ENOENT;
	}
	template<class FN_T>
	inline ssize_t cpoll::PollUpdate(fd_t fd, uint events, FN_T&& callback) {
		if (auto&& it{ fdHandlers.find(fd) }; it != fdHandlers.end()) {
			fdHandlers[fd] = std::move(callback);
			return PollUpdate(fd, events);
		}
		return -ENOENT;
	}
}
