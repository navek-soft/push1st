#pragma once
#include "cserver.h"

namespace inet {
	class cunixserver : public inet::cserver {
	public:
		static inline size_t DefaultRecvTimeout{ 1500 }, DefaultSendTimeout{ 1500 };

		cunixserver(const std::string& nameServer, bool enableClientNonBlockingMode, std::time_t clientActivityTimeout);
		virtual ~cunixserver() { ; }
		virtual void Listen(const std::shared_ptr<cpoll>& poll) override;
		
	protected:
		virtual std::shared_ptr<inet::cunixserver> UnixSelf() = 0;
		virtual inline std::shared_ptr<cserver> GetInstance() { return std::dynamic_pointer_cast<cserver>(UnixSelf()); }
		virtual ssize_t OnUnixAccept(fd_t fd, const sockaddr_storage& sa, const inet::ssl_t& ssl, const std::weak_ptr<inet::cpoll>& poll) = 0;
		ssize_t UnixListen(const std::string_view& listenHostPort, bool nonblock, int maxlisten, const ssl_ctx_t& sslContext);
		void UnixClose();
		virtual ssize_t OnConnect(fd_t fd) = 0;
		virtual ssize_t OnRead(fd_t fd) = 0;
		virtual void OnDisconnect(fd_t fd, ssize_t err) = 0;
	private:
		virtual void OnAccept(fd_t, uint, std::weak_ptr<cpoll> poll) override;
		virtual inline void OnAcceptSSL(fd_t, uint, std::weak_ptr<cpoll> poll) { ; }
		fd_t srvFd{ -1 };
		bool cliNonBlock{ false };
	};
}

using serverunix_t = inet::cunixserver;