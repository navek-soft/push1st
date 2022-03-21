#pragma once
#include <thread>
#include <shared_mutex>
#include <list>
#include "../ci/csyslog.h"
#include "cinet.h"

class cserver;
class cclient;

class cpoll : protected csyslog, public std::enable_shared_from_this<cpoll>
{
public:
	static inline size_t MaxEvents{ 256 };

	cpoll(std::time_t timerMilliseconds = 1000);
	~cpoll();
	template<typename T>
	inline void Listen(const std::shared_ptr<T>& server);
	template<typename CLI_T>
	inline ssize_t Add(const std::shared_ptr<CLI_T>& cli, cevent& event);
	template<typename CLI_T>
	inline ssize_t Add(const std::shared_ptr<CLI_T>& cli);
	inline ssize_t Add(cevent& event);
	inline ssize_t Mod(cevent& event);
	inline ssize_t Del(cevent& event);
	void Join();
private:
	static void PollThread(cpoll* self);
	void OnPollTimer(int, uint event);

private:
	int pollFd{ -1 };
	std::thread pollThread;
	std::list<std::weak_ptr<cpipe>> pollClients;
	std::shared_mutex pollGuard;
	cevent timerEvent;
};

template<typename T>
inline void cpoll::Listen(const std::shared_ptr<T>& server) {
	return server->Listen(shared_from_this());
}
template<typename CLI_T>
inline ssize_t cpoll::Add(const std::shared_ptr<CLI_T>& client, cevent& event) {
	{
		std::unique_lock<decltype(pollGuard)> lock(pollGuard);
		pollClients.emplace_back(std::dynamic_pointer_cast<cpipe>(client));
	}
	return Add(event);
}

template<typename CLI_T>
inline ssize_t cpoll::Add(const std::shared_ptr<CLI_T>& client) {
	std::unique_lock<decltype(pollGuard)> lock(pollGuard);
	pollClients.emplace_back(std::dynamic_pointer_cast<cpipe>(client));
}

inline ssize_t cpoll::Add(cevent& event) {
	return epoll_ctl(pollFd, EPOLL_CTL_ADD, (int)(event), (epoll_event*)event) == 0 ? 0 : -errno;
}
inline ssize_t cpoll::Mod(cevent& event) {
	return epoll_ctl(pollFd, EPOLL_CTL_MOD, (int)(event), (epoll_event*)event) == 0 ? 0 : -errno;
}
inline ssize_t cpoll::Del(cevent& event) {
	return epoll_ctl(pollFd, EPOLL_CTL_DEL, (int)(event), (epoll_event*)event) == 0 ? 0 : -errno;
}