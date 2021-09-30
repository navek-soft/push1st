#include "chttpserver.h"

using namespace inet;

ssize_t chttpserver::OnTcpAccept(fd_t fd, const sockaddr_storage& sa, const inet::ssl_t& ssl, const std::weak_ptr<inet::cpoll>& poll) {
	ssize_t res{ -1 };
	if (auto&& self{ poll.lock() }; self) {
		return self->PollAdd(fd, EPOLLIN | EPOLLRDHUP | EPOLLERR, std::bind(&chttpserver::HttpData, this, std::placeholders::_1, std::placeholders::_2, sa, ssl, poll));
	}
	return res;
}


void chttpserver::HttpData(fd_t fd, uint events, const sockaddr_storage& sa, const inet::ssl_t& ssl, const std::weak_ptr<inet::cpoll>& poll) {
	ssize_t res{ -1 };
	if (auto&& self{ poll.lock() }; self) {
		std::string_view method; http::uri_t path; http::headers_t headers;  std::string request, content;
		inet::csocket so(fd, sa, ssl, poll);
		if (res = HttpReadRequest(so, method, path, headers, request, content, HttpMaxHeaderSize); res == 0) {
			OnHttpRequest(so, method, path, headers, std::move(request), std::move(content));
		}
		else {
			so.SocketClose();
		}
		return;
	}
	inet::Close(fd);
}

chttpserver::chttpserver(const std::string& name, const std::string& HostPort, const inet::ssl_ctx_t& SslCtx, size_t httpMaxHeaderSize) :
	inet::ctcpserver{ name, false, 1500, 2000, true }, HttpMaxHeaderSize{ httpMaxHeaderSize }
{
	TcpListen(HostPort, true, true, false, 4096, SslCtx);
}

chttpserver::~chttpserver() {
	TcpClose();
}
