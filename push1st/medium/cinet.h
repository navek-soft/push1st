#pragma once
#include <ctime>
#include <memory>
#include <string>
#include <functional>
#include <sys/socket.h>
#include <sys/epoll.h>

struct ssl_st;
struct ssl_ctx_st;

using ssl_t = std::shared_ptr<struct ssl_st>;
using ssl_ctx_t = std::shared_ptr<struct ssl_ctx_st>;

class cpoller {
public:
	virtual ~cpoller() = default;
protected:
	friend class cpoll;
	virtual bool OnCheck(std::time_t time) = 0;
};

class cpoll;
using handler_t = std::function<void(int, uint, const std::shared_ptr<cpoll>&)>;

class cevent {
public:
	cevent() = default;
	cevent(int h, uint evnts, const handler_t& fn) : fd{ h }, callback{ fn }, event{ .events = evnts, .data {.ptr = this } }{; }
	cevent(const handler_t& fn) : callback{ fn }, event{ .events = 0, .data {.ptr = this } }{; }
	inline explicit operator epoll_event* () { return &event; }
	inline cevent& operator ()(int h, uint evnts) { fd = h; event.events = evnts; return *this; }
	inline explicit operator bool() { return fd > 0; }
	inline explicit operator int() { return fd; }
	inline bool operator == (int h) { return fd == h; }
	inline bool operator != (int h) { return fd != h; }
	inline cevent& operator = (const cevent& ev) { fd = ev.fd; callback = std::move(ev.callback); event.events = ev.event.events; return *this; }
private:
	friend class cpoll;
	inline void trigger(uint evnts, const std::shared_ptr<cpoll>& poll) { callback(fd, evnts, poll); }
private:
	int fd{ -1 };
	handler_t callback;
	epoll_event event{ .events = 0, .data {.ptr = this } };
};