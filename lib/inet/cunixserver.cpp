#include "cunixserver.h"
#include <csignal>
#include "cepoll.h"

using namespace inet;

void cunixserver::OnAccept(fd_t fd, uint events, std::weak_ptr<cpoll> poll) {

	if (events == EPOLLIN) {
		sockaddr_storage sa;
		ssize_t res{ -1 };
		fd_t cli{ -1 };
		if (res = inet::UnixAccept(fd, cli, sa, cliNonBlock); res == 0) {
			if ((res = OnUnixAccept(cli, sa, {}, poll)) == 0) {
				return;
			}
			fprintf(stderr, "[ UNIXSERVER:%s(%ld) ] connection error: %s\n", NameOf(), cli, inet::GetErrorStr(res));
			inet::Close(cli);
		}
		else if(res != -EAGAIN) {
			fprintf(stderr, "[ UNIXSERVER:%s(%ld) ] accept error: %s\n", NameOf(), fd, inet::GetErrorStr(res));
		}
		return;
	}
	fprintf(stderr, "[ UNIXSERVER:%s(%ld) ] server poll error: %s%s%s%s\n", NameOf(), fd, (events & EPOLLIN) ? "EPOLLIN " : "", (events & EPOLLRDHUP) ? "EPOLLRDHUP " : "", (events & EPOLLHUP) ? "EPOLLHUP " : "", (events & EPOLLERR) ? "EPOLLERR " : "");
	inet::Close(fd);
	raise(SIGPIPE);
}

void cunixserver::Listen(const std::shared_ptr<cpoll>& poll) {
	if (srvFd > 0) {
		ssize_t res{ -1 };
		if (res = poll->PollAdd(srvFd, EPOLLIN | EPOLLEXCLUSIVE, std::bind(&cunixserver::OnAccept, this, std::placeholders::_1, std::placeholders::_2, poll->weak_from_this())); res == 0) {
			return;
		}
		fprintf(stderr, "[ UNIXSERVER:%s(%ld) ] Listen error: %s\n", NameOf(), std::strerror((int)-res));
		throw std::runtime_error("UNIX server fail");
	}
}

ssize_t cunixserver::UnixListen(const std::string_view& listenFilePath, bool nonblock, int maxlisten) {
	ssize_t res{ -1 };
	if (sockaddr_storage sa; (res = inet::GetSockAddr(sa, listenFilePath, "0", AF_UNIX)) == 0 and (res = inet::UnixServer(srvFd,sa, nonblock, maxlisten)) == 0) {
		return res;
	}
	inet::Close(srvFd);
	return res;
}

void cunixserver::UnixClose() {
	inet::Close(srvFd);
}


cunixserver::cunixserver(const std::string& nameServer, bool enableClientNonBlockingMode, std::time_t clientActivityTimeout) :
	cserver{ nameServer },
	cliNonBlock{ enableClientNonBlockingMode }
{
	
}

cunixserver::~cunixserver() {
	UnixClose();
}