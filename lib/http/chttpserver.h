#pragma once
#include "../inet/ctcpserver.h"
#include "chttpconn.h"

namespace inet {

	class chttpserver : public inet::ctcpserver, public http::cconnection {
	public:
		chttpserver(const std::string& name, const std::string& HostPort, size_t httpMaxHeaderSize = 8192);
		chttpserver(const std::string& name, const std::string& HostPort, const std::string& SslCert, const std::string& SslKey, size_t httpMaxHeaderSize = 8192);
		virtual ~chttpserver();
	protected:
		virtual void OnHttpRequest(const inet::csocket& fd, const std::string_view& method, const http::path_t& path, const http::params_t& args, const http::headers_t& headers, std::string&& request, std::string&& content) = 0;
		virtual inline std::shared_ptr<inet::ctcpserver> TcpSelf() = 0;
	private:
		virtual ssize_t OnTcpAccept(fd_t fd, const sockaddr_storage& sa, const inet::ssl_t& ssl, const std::weak_ptr<inet::cpoll>& poll) override;

	private:
		size_t HttpMaxHeaderSize{ 8192 };
	};
}