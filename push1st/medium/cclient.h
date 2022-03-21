#pragma once
#include <memory>
#include "../ci/csyslog.h"
#include <sys/ioctl.h>
#include "cinet.h"

class cpoll;

class cclient : public csyslog, public cpoller {
protected:
	friend class cserver;
	cclient(const cclient&& cli) = delete;
	cclient(const cclient& cli) = delete;
	cclient(cclient&& cli);
	cclient(int fd, std::unique_ptr<sockaddr_storage>&& sa, const std::shared_ptr<cpoll>& poll, const ssl_t& ssl);
	inline ssize_t Send(const void* data, size_t length, uint flags = 0, const sockaddr_storage* sa = nullptr) const;
	inline ssize_t Recv(void* data, size_t length, uint flags = 0, sockaddr_storage* sa = nullptr) const;
	void Close();
protected:
	virtual void OnAccept() { ; }
	virtual void OnError(error_t err) { Close(); }
public:
	virtual ~cclient();
private:
	ssize_t send_nossl(const void* data, size_t length, uint flags, const sockaddr_storage* sa) const;
	ssize_t send_ssl(const void* data, size_t length, uint flags, const  sockaddr_storage* sa) const;
	ssize_t recv_nossl(void* data, size_t length, uint flags, sockaddr_storage* sa) const;
	ssize_t recv_ssl(void* data, size_t length, uint flags, sockaddr_storage* sa) const;
private:
	int cliFd{ -1 };
	ssize_t(cclient::* send_fn)(const void* data, size_t length, uint flags, const sockaddr_storage* sa) const { &cclient::send_nossl };
	ssize_t(cclient::* recv_fn)(void* data, size_t length, uint flags, sockaddr_storage* sa) const { &cclient::recv_nossl };
	ssl_t cliSsl{ nullptr };
	std::unique_ptr<sockaddr_storage> cliSa;
};

inline ssize_t cclient::Send(const void* data, size_t length, uint flags = 0, const sockaddr_storage* sa = nullptr) const {
	if (auto nsent{ (this->*send_fn)(data, length, flags, sa) }; nsent > 0 and flags | MSG_DONTWAIT) {
		if (unsigned long  nsize{ 0 }; ioctl(cliFd, TIOCOUTQ, &nsize) != -1) {
			if (nsize <= (length << 1)) {
				return nsent;
			}
			return -ECOMM;
		}
		return -errno;
	}
	else {
		return nsent;
	}
}

inline ssize_t cclient::Recv(void* data, size_t length, uint flags, sockaddr_storage* sa) const {
	return (this->*recv_fn)(data, length, flags, sa);
	
}
