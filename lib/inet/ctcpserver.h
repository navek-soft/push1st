#pragma once
#include "cserver.h"

namespace inet {
	class ctcpserver : public inet::cserver {
	public:
		ctcpserver(const std::string& nameServer, bool enableClientNonBlockingMode, std::time_t timeoutMsRecv = 1500, std::time_t timeoutMsSend = 2000, bool enableKeepAlive = false, int KeepAliveRetries = 3, int KeepAliveTimeout = 2, int KeepAliveInterval = 2);
		virtual ~ctcpserver();
		virtual void Listen(const std::shared_ptr<cpoll>& poll) override;
	protected:
		ssize_t TcpListen(const std::string_view& listenHostPort, bool reuseaddress, bool reuseport, bool nonblock, int maxlisten, const ssl_ctx_t& sslContext = { nullptr });
		void TcpClose();
		virtual ssize_t OnTcpAccept(fd_t fd, const sockaddr_storage& sa, const inet::ssl_t& ssl, const std::weak_ptr<inet::cpoll>& poll) = 0;
		virtual std::shared_ptr<inet::ctcpserver> TcpSelf() = 0;
		virtual inline std::shared_ptr<cserver> GetInstance() { return std::dynamic_pointer_cast<cserver>(TcpSelf()); }
	private:
		virtual void OnAccept(fd_t, uint, std::weak_ptr<cpoll> poll) override;
		virtual void OnAcceptSSL(fd_t, uint, std::weak_ptr<cpoll> poll) override;
	private:
		fd_t srvFd{ -1 };
		bool cliNonBlock{ false };
		std::time_t DefaultRecvTimeout{ 1500 }, DefaultSendTimeout{ 1500 };
		bool DefaultKeepAliveEnable{ false };
		int DefaultKeepAliveRetries{ 3 }, DefaultKeepAliveTimeout{ 2 }, DefaultKeepAliveInterval{ 2 };
		ssl_ctx_t srvSslContext;
	};
}
