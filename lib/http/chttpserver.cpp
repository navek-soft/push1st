#include "chttpserver.h"

using namespace inet;

ssize_t chttpserver::OnTcpAccept(fd_t fd, const sockaddr_storage& sa, const inet::ssl_t& ssl, const std::weak_ptr<inet::cpoll>& poll) {
	/* Read HTTP request */
	ssize_t res{ -1 };
	if (auto&& self{ poll.lock() }; self) {
		std::string_view method; http::path_t path; http::params_t args; http::headers_t headers;  std::string request, content;
		inet::csocket so(fd, sa, ssl, poll);

		if (res = HttpReadRequest(so, method, path, args, headers, request, content, HttpMaxHeaderSize); res == 0) {
			OnHttpRequest(so, method, path, args, headers, std::move(request), std::move(content));
			return 0;
		}
		so.SocketDetach();
	}
	return res;
}

chttpserver::chttpserver(const std::string& name, const std::string& HostPort, size_t httpMaxHeaderSize) :
	inet::ctcpserver{ name, false, 1500, 2000, true }, HttpMaxHeaderSize{ httpMaxHeaderSize }
{
	TcpListen(HostPort, true, true, false, 4096, {});
}

chttpserver::chttpserver(const std::string& name, const std::string& HostPort, const std::string& SslCert, const std::string& SslKey, size_t httpMaxHeaderSize) :
	inet::ctcpserver{ name, false, 1500, 2000, true }, HttpMaxHeaderSize{ httpMaxHeaderSize }
{
	if (inet::ssl_ctx_t sslCtx; inet::SslCreateContext(sslCtx, SslCert, SslKey, true) == 0) {
		TcpListen(HostPort, true, true, false, 4096, {});
	}
	else {
		throw std::runtime_error("HTTP-SERVER: Unable to init SSL certificate ");
	}
}

chttpserver::~chttpserver() {
	TcpClose();
}
