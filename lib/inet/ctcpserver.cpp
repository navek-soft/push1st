#include "ctcpserver.h"
#include <csignal>
#include "cepoll.h"

using namespace inet;

void ctcpserver::OnAccept(fd_t fd, uint events, std::weak_ptr<cpoll> poll) {
	if (events == EPOLLIN) {
		sockaddr_storage sa;
		ssize_t res{ -1 };
		int cli{ -1 };
		if (res = inet::TcpAccept(fd, cli, sa, cliNonBlock); res == 0) {
			inet::SetRecvTimeout(cli, DefaultRecvTimeout);
			inet::SetSendTimeout(cli, DefaultSendTimeout);
			inet::SetKeepAlive(cli, DefaultKeepAliveEnable, DefaultKeepAliveRetries, DefaultKeepAliveTimeout, DefaultKeepAliveInterval);
			if ((res = OnTcpAccept(cli, sa, {}, poll)) == 0) {
				return;
			}
			fprintf(stderr, "[ TCPSERVER:%s(%ld) ] connection error: %s\n", NameOf(), cli, inet::GetErrorStr(res));
			inet::Close(cli);
		}
		else {
			fprintf(stderr, "[ TCPSERVER:%s(%ld) ] accept error: %s\n", NameOf(), fd, inet::GetErrorStr(res));
		}
		return;
	}
	fprintf(stderr, "[ TCPSERVER:%s(%ld) ] server poll error: %s%s%s%s\n", NameOf(), fd, (events & EPOLLIN) ? "EPOLLIN " : "", (events & EPOLLRDHUP) ? "EPOLLRDHUP " : "", (events & EPOLLHUP) ? "EPOLLHUP " : "", (events & EPOLLERR) ? "EPOLLERR " : "");
	inet::Close(fd);

	raise(SIGPIPE);
}

void ctcpserver::OnAcceptSSL(fd_t fd, uint events, std::weak_ptr<cpoll> poll) {
	if (events == EPOLLIN) {
		sockaddr_storage sa;
		ssize_t res{ -1 };
		int cli{ -1 };
		ssl_t cliSSL;
		if ((res = inet::TcpAccept(fd, cli, sa, cliNonBlock)) == 0 and (res = inet::SslAcceptConnection(cli, srvSslContext, cliSSL)) == 0) {
			inet::SetRecvTimeout(cli, DefaultRecvTimeout);
			inet::SetSendTimeout(cli, DefaultSendTimeout);
			inet::SetKeepAlive(cli, DefaultKeepAliveEnable, DefaultKeepAliveRetries, DefaultKeepAliveTimeout, DefaultKeepAliveInterval);

			if ((res = OnTcpAccept(cli, sa, cliSSL, poll)) == 0) {
				return;
			}

			fprintf(stderr, "[ TCPSERVER:%s(%ld) ] connection error: %s\n", NameOf(), cli, inet::GetErrorStr(res));
		}
		else {
			fprintf(stderr, "[ TCPSERVER:%s(%ld) ] accept error: %s\n", NameOf(), fd, inet::GetErrorStr(res));
		}
		cliSSL.reset();
		inet::Close(cli);
		return;
	}
	fprintf(stderr, "[ TCPSERVER:%s(%ld) ] server poll error: %s%s%s%s\n", NameOf(), fd, (events & EPOLLIN) ? "EPOLLIN " : "", (events & EPOLLRDHUP) ? "EPOLLRDHUP " : "", (events & EPOLLHUP) ? "EPOLLHUP " : "", (events & EPOLLERR) ? "EPOLLERR " : "");
	inet::Close(fd);

	raise(SIGPIPE);
}

void ctcpserver::Listen(const std::shared_ptr<cpoll>& poll) {
	if (srvFd > 0) {
		if (srvSslContext) {
			poll->PollAdd(srvFd, EPOLLIN | EPOLLRDHUP | EPOLLEXCLUSIVE, std::bind(&ctcpserver::OnAcceptSSL, this, std::placeholders::_1, std::placeholders::_2,poll->weak_from_this()));
		}
		else {
			poll->PollAdd(srvFd, EPOLLIN | EPOLLRDHUP | EPOLLEXCLUSIVE, std::bind(&ctcpserver::OnAccept, this, std::placeholders::_1, std::placeholders::_2, poll->weak_from_this()));
			//poll->PollAdd(srvFd, EPOLLIN | EPOLLRDHUP, { TcpSelf() }, &ctcpserver::OnAccept);
		}
	}
}

ssize_t ctcpserver::TcpListen(const std::string_view& listenHostPort, bool reuseaddress, bool reuseport, bool nonblock, int maxlisten, const ssl_ctx_t& sslContext) {
	srvSslContext = sslContext;
	ssize_t res{ -1 };
	if (sockaddr_storage sa; (res = inet::GetSockAddr(sa, listenHostPort, "0", AF_INET)) == 0 and (res = inet::TcpServer(srvFd, sa, reuseaddress, reuseport, nonblock, maxlisten) == 0)) {
		return res;
	}
	inet::Close(srvFd);
	return res;
}

void ctcpserver::TcpClose() {
	inet::Close(srvFd);
}

ctcpserver::ctcpserver(const std::string& nameServer, bool enableClientNonBlockingMode, std::time_t timeoutMsRecv, std::time_t timeoutMsSend, bool enableKeepAlive, int KeepAliveRetries, int KeepAliveTimeout, int KeepAliveInterval) :
	cserver{ nameServer },
	cliNonBlock{ enableClientNonBlockingMode }, DefaultRecvTimeout{ timeoutMsRecv }, DefaultSendTimeout{ timeoutMsSend },
	DefaultKeepAliveEnable{ enableKeepAlive }, DefaultKeepAliveRetries{ KeepAliveRetries }, DefaultKeepAliveTimeout{ KeepAliveTimeout }, DefaultKeepAliveInterval{ KeepAliveInterval }
{

}

ctcpserver::~ctcpserver() {
	TcpClose();
}