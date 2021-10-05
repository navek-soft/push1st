#include "cepoll.h"
#include <csignal>
#include <unistd.h>
#include <vector>
#include <functional>

using namespace inet;

inline void cpoll::Gc() {
	while(!fdQueueGC.empty()) {
		auto fd{ fdQueueGC.front() };
		fdQueueGC.pop_front();
		if (auto res = inet::GetErrorNo(fd); res != 0) {
			std::unique_lock<decltype(fdLock)> lock(fdLock);
			if (auto&& hFd{ fdHandlers.find(fd) }; hFd != fdHandlers.end())
			{
				if (hFd->second) {
					hFd->second(fd, EPOLLRDHUP);
					break;
				}
			}
		}
		else if (auto&& hFd{ fdHandlers.find(fd) }; hFd != fdHandlers.end()) {
			fdQueueGC.emplace_back(fd);
		}
		break;
	}
}

void cpoll::PollThread(std::shared_ptr<cpoll> self, int numEventsMax, int msTimeout) {
	std::vector<struct epoll_event> events_list(numEventsMax);
	pthread_setname_np(pthread_self(), self->NameOf());
	try {
		sigset_t epoll_sig_mask;
		sigset_t sigset; sigemptyset(&sigset);
		sigaddset(&sigset, SIGPIPE);
		sigaddset(&sigset, SIGHUP);
		pthread_sigmask(SIG_BLOCK, &epoll_sig_mask, NULL);
		ssize_t nevents{ 0 };
		while (1) {
			if (nevents = epoll_wait((int)self->fdPoll, (struct epoll_event*)events_list.data(), (int)events_list.size(), msTimeout); nevents > 0) {

				while (nevents--) {
					if (auto&& hFd{ self->fdHandlers.find(events_list[nevents].data.fd) }; hFd != self->fdHandlers.end())
					{
						if (hFd->second) {
							hFd->second(events_list[nevents].data.fd, events_list[nevents].events);
							continue;
						}
					}
					fprintf(stderr, "[ SERVER:%s ] Unhandled socket ( %ld )\n", self->NameOf(), events_list[nevents].data.fd);
					int fd{ events_list[nevents].data.fd };
					inet::Close(fd);
				}
				continue;
			}
			else if (!nevents) {
				continue;
			}


#ifdef DEBUG
			if (errno == EINTR) { 
				continue; 
			}
#endif // DEBUG
			fprintf(stderr, "[ SERVER:%s ] epoll error (%s)\n", self->NameOf(), inet::GetErrorStr(errno));
			raise(SIGHUP);
			break;
		}
	}
	catch (std::exception& ex) {
		fprintf(stderr, "[ SERVER:%s ] Exception (%s)\n", self->NameOf(), ex.what());
		raise(SIGHUP);
	}
	catch (...) {
		auto eptr = std::current_exception();
		fprintf(stderr, "[ SERVER:%s ] Unhandled exception\n", self->NameOf());
		raise(SIGHUP);
	}
}

ssize_t cpoll::Listen(int numEventsMax, int pollTimeoutMs) {
	if (fdPoll > 0 and !pollThread.joinable()) {
		pollThread = std::thread(std::bind(cpoll::PollThread, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3), Self(), numEventsMax, pollTimeoutMs);
		return pollThread.joinable() ? 0 : -EBADE;
	}
	return -EALREADY;
}

void cpoll::Join() {
	if (fdPoll > 0) { ::close(fdPoll); fdPoll = -1; }
	if (pollThread.joinable()) { pollThread.join(); }
}

ssize_t cpoll::PollUpdate(fd_t fd, uint events) {
	struct epoll_event ev { .events = events, .data = { .fd = fd } };
	return epoll_ctl(fdPoll, EPOLL_CTL_MOD, fd, &ev) == 0 ? 0 : -errno;
}

void cpoll::PollDelete(fd_t& fd) {
	if (fd > 0) {
		epoll_ctl(fdPoll, EPOLL_CTL_DEL, fd, nullptr);
		::shutdown(fd, SHUT_RDWR);
		::close(fd);
		std::unique_lock<decltype(fdLock)> lock(fdLock);
		fdHandlers.erase(fd);
		fd = -1;
	}
/*
	std::string dump;
	for (auto&& f : fdHandlers) {
		dump.append(":").append(std::to_string(f.first)).append(" ");
	}
	printf("PollDelete #%ld %s\n", fdPoll, dump.c_str());
	*/
}

cpoll::cpoll() {
	if (fdPoll = epoll_create1(EPOLL_CLOEXEC); fdPoll > 0) {
		return;
	}
	throw std::runtime_error(std::string{ "cservice::cservice() " } + strerror(errno));
}

cpoll::~cpoll() {
	Join();
}