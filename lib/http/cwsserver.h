#pragma once
#include "../inet/cinet.h"
#include "../inet/ctcpserver.h"
#include "chttpconn.h"
#include "cwebsocket.h"

namespace inet {
	class cwsserver : public inet::ctcpserver, public inet::chttpconnection {
	public:
		cwsserver(const std::string& name, const std::string& HostPort, const inet::ssl_ctx_t& SslCtx, size_t httpMaxHeaderSize = 8192);
		virtual ~cwsserver();
	protected:
		virtual void WsAccept(fd_t fd, uint events, const sockaddr_storage& sa, const inet::ssl_t& ssl, const std::weak_ptr<inet::cpoll>& poll);
		virtual ssize_t WsUpgrade(const inet::csocket& fd, const http::path_t& path, const http::params_t& args, const http::headers_t& headers, std::string&& request, std::string&& content);
		virtual inet::socket_t OnWsUpgrade(const inet::csocket& fd, const http::path_t& path, const http::params_t& args, const http::headers_t& headers, std::string&& request, std::string&& content) = 0;
	private:
		virtual inline std::shared_ptr<inet::ctcpserver> TcpSelf() = 0;
		virtual ssize_t OnTcpAccept(fd_t fd, const sockaddr_storage& sa, const inet::ssl_t& ssl, const std::weak_ptr<inet::cpoll>& poll);
		void WsData(fd_t fd, uint events, inet::socket_t so, const std::weak_ptr<inet::cpoll>& poll);

	private:
		size_t HttpMaxHeaderSize{ 8192 };
	};
}