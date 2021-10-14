#pragma once
#include "../inet/ctcpserver.h"
#include "chttpconn.h"

namespace inet {

	class chttpserver : public inet::ctcpserver, public inet::chttpconnection {
	public:
		chttpserver(const std::string& name, const std::string& HostPort, const inet::ssl_ctx_t& SslCtx, size_t httpMaxHeaderSize = 8192);
		virtual ~chttpserver();
	protected:
		virtual void OnHttpRequest(const inet::csocket& fd, const std::string_view& method, const http::uri_t& path, const http::headers_t& headers, const std::string& request, const std::string_view& content) = 0;
		virtual void OnHttpError(const inet::csocket& fd, ssize_t err) = 0;
		virtual inline std::shared_ptr<inet::ctcpserver> TcpSelf() = 0;
	private:
		virtual ssize_t OnTcpAccept(fd_t fd, const sockaddr_storage& sa, const inet::ssl_t& ssl, const std::weak_ptr<inet::cpoll>& poll) override;
		void HttpData(fd_t fd, uint events, const sockaddr_storage& sa, const inet::ssl_t& ssl, const std::weak_ptr<inet::cpoll>& poll);
	private:
		size_t HttpMaxHeaderSize{ 8192 };
	};
}