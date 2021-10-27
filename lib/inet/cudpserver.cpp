#include "cudpserver.h"
#include <csignal>
#include "cepoll.h"

using namespace inet;

void cudpserver::OnAccept(fd_t fd, uint events, std::weak_ptr<cpoll> poll) {
	if (events == EPOLLIN) {
		OnUdpData(fd, {}, poll);
		return;
	}
	fprintf(stderr, "[ UDPSERVER:%s(%ld) ] server poll error: %s%s%s%s\n", NameOf(), fd, (events & EPOLLIN) ? "EPOLLIN " : "", (events & EPOLLRDHUP) ? "EPOLLRDHUP " : "", (events & EPOLLHUP) ? "EPOLLHUP " : "", (events & EPOLLERR) ? "EPOLLERR " : "");
	inet::Close(fd);

	raise(SIGPIPE);
}

void cudpserver::Listen(const std::shared_ptr<cpoll>& poll) {
	if (srvFd > 0) {
		ssize_t res{ 0 };
		if (res = poll->PollAdd(srvFd, EPOLLIN | EPOLLEXCLUSIVE, std::bind(&cudpserver::OnAccept, this, std::placeholders::_1, std::placeholders::_2, poll->weak_from_this())); res == 0) {
			return;
		}
		fprintf(stderr, "[ UDPSERVER:%s(%ld) ] Listen error: %s\n", NameOf(), std::strerror((int)-res));
		throw std::runtime_error("UDP server fail");
	}
}


ssize_t cudpserver::UdpListen(const std::string_view& listenHostPort, bool reuseaddress, bool reuseport, bool nonblock) {
	ssize_t res{ -1 };
	if (sockaddr_storage sa; (res = inet::GetSockAddr(sa, listenHostPort, "0", AF_INET)) == 0 and (res = inet::UdpServer(srvFd, sa, reuseaddress, reuseport, nonblock)) == 0) {
		return res;
	}
	inet::Close(srvFd);
	return res;
}


void cudpserver::UdpClose() {
	inet::Close(srvFd);
}


cudpserver::cudpserver(const std::string& nameServer) :
	inet::cserver{ nameServer }
{

}

cudpserver::~cudpserver() {
	UdpClose();
}