#pragma once
#include "cserver.h"

namespace inet {
	class cunixserver : public inet::cserver {
	public:
		static inline size_t DefaultRecvTimeout{ 1500 }, DefaultSendTimeout{ 1500 };

		cunixserver(const std::string& nameServer, bool enableClientNonBlockingMode, std::time_t clientActivityTimeout) : cserver{ nameServer }, cliNonBlock{ enableClientNonBlockingMode } {; }
		virtual ~cunixserver() { ; }
		void Close(ssize_t fd);
	private:
		virtual std::shared_ptr<cserver> getServerInstance() = 0;
	protected:
		virtual const char* Name() { return "srv:unix"; }
		ssize_t Create(const std::string_view& listenHostPort, bool nonblock, int maxlisten);
		virtual ssize_t OnAllow(fd_t fd, const sockaddr_storage& sa) = 0;
		virtual ssize_t OnConnect(fd_t fd) = 0;
		virtual ssize_t OnRead(fd_t fd) = 0;
		virtual void OnDisconnect(fd_t fd, ssize_t err) = 0;
	private:
		ssize_t unixOnRead(fd_t fd, uint events);
		ssize_t unixOnAccept(fd_t fd, uint events);
		bool cliNonBlock{ false };
	};
}

using serverunix_t = inet::cunixserver;