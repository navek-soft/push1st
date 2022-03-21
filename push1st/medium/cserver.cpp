#include "cserver.h"
#include <cstring>
#include <regex>
#include <netinet/in.h>
#include <unistd.h>

void cserver::TcpServer(const std::string& listenIfacePort, const ssl_ctx_t& sslCtx, const std::function<void(cclient&&)>& AcceptFn, int maxListen, int af, bool nonblock, bool reuseaddress, bool reuseport) {
	ssize_t res{ 0 };
	int fd = -1;
	sockaddr_storage sa;

	SockAddress(sa, listenIfacePort, af);

	if (fd = ::socket(sa.ss_family, SOCK_CLOEXEC | SOCK_STREAM | (nonblock ? SOCK_NONBLOCK : 0), 0); fd <= 0) { throw std::runtime_error(std::strerror(errno)); }
	if (int nvalue = 1; reuseaddress && setsockopt((int)fd, SOL_SOCKET, SO_REUSEADDR, &nvalue, sizeof(nvalue)) != 0) { res = -errno; ::close((int)fd); fd = -1; throw std::runtime_error(std::strerror(errno)); }
	if (int nvalue = 1; reuseport && setsockopt((int)fd, SOL_SOCKET, SO_REUSEPORT, &nvalue, sizeof(nvalue)) != 0) { res = -errno; ::close((int)fd); fd = -1; throw std::runtime_error(std::strerror(errno)); }
	if (::bind((int)fd, (sockaddr*)&sa, sizeof(sockaddr_storage)) != 0) { res = -errno; ::close((int)fd); fd = -1; throw std::runtime_error(std::strerror(errno));; }
	if (::listen((int)fd, maxListen) != 0) { res = -errno; ::close((int)fd); fd = -1; throw std::runtime_error(std::strerror(errno)); }

	if (sslCtx) {
		srvFds.emplace_back(cevent{ fd, EPOLLIN | EPOLLET, [ctx = sslCtx, fn = AcceptFn](int fd, uint events, const std::shared_ptr<cpoll>& poll) {
		} });
	}
	else {
		srvFds.emplace_back(cevent{ fd, EPOLLIN | EPOLLET, [fn = AcceptFn](int fd, uint events, const std::shared_ptr<cpoll>& poll) {
		} });
	}
}

void cserver::UdpServer(const std::string& listenIfacePort, const std::function<void(cclient&&)>& RecvFn, int af, bool nonblock, bool reuseaddress, bool reuseport) {
}

void cserver::UnixServer(const std::string& listenIfacePort, const std::function<void(cclient&&)>& AcceptFn, int maxListen, bool nonblock) {
}

cserver::cserver(const std::string& name) :
	csyslog{ name }
{ 
	;
}

cserver::~cserver() {

}

void cserver::SockAddress(sockaddr_storage& sa, const std::string& listenIfacePort, int af) {

	static const std::regex re(R"((?:([\w*-]+):)?(\d+))");

	sockaddr_storage sa;
	bzero(&sa, sizeof(sa));

	std::string bindIface, bindPort;

	std::smatch match;
	if (std::regex_search(listenIfacePort, match, re) && match.size() > 1) {
		if (bindIface = match.str(1); bindIface == "*") {
			bindIface.clear();
		}
		bindPort = match.str(2);
	}
	else {
		throw std::runtime_error("server( Invalid server iface:port )");
	}

	switch (af)
	{
	case AF_INET:
		sa.ss_family = AF_INET;
		((sockaddr_in&)sa).sin_port = htons((uint16_t)std::stol(bindPort));
		break;
	case AF_INET6:
		sa.ss_family = AF_INET6;
		((sockaddr_in6&)sa).sin6_port = htons((uint16_t)std::stol(bindPort));
		break;
	case AF_UNIX:
	default:
		throw std::runtime_error(std::strerror(ENOTSUP));
		break;
	}
}