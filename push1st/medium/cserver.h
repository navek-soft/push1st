#pragma once
#include "cinet.h"
#include "../ci/cconfig.h"
#include "../ci/csyslog.h"
#include "cclient.h"
#include <vector>

class cpoll;

class cserver : public csyslog {
public:
	virtual ~cserver();
	virtual void Listen(const std::shared_ptr<cpoll>& poll) = 0;
	virtual void Join() = 0;
protected:
	cserver(const std::string& name);
	void TcpServer(const std::string& listenIfacePort, const ssl_ctx_t& sslCtx, const std::function<void(cclient&&)>& AcceptFn, int maxListen, int af = AF_INET, bool nonblock = true, bool reuseaddress = true, bool reuseport = true);
	void UdpServer(const std::string& listenIfacePort, const std::function<void(cclient&&)>& RecvFn, int af = AF_INET, bool nonblock = true, bool reuseaddress = true, bool reuseport = true);
	void UnixServer(const std::string& listenIfacePort, const std::function<void(cclient&&)>& AcceptFn, int maxListen, bool nonblock = true);
private:
	inline void SockAddress(sockaddr_storage& sa, const std::string& listenIfacePort, int af);
	std::vector<cevent> srvFds;
};

using server_t = std::shared_ptr<cserver>;
