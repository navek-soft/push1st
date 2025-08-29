#pragma once
#include "cserver.h"
#include "csocket.h"

namespace inet {
	class cudpserver : public inet::cserver {
	public:
		cudpserver(const std::string& nameServer);
		virtual ~cudpserver();
		virtual void Listen(const std::shared_ptr<cpoll>& poll) override;
	protected:
		inline inet::csocket Fd() { return { srvFd, nullptr }; }
		ssize_t UdpListen(const std::string_view& iface, const std::string_view& port, bool reuseaddress, bool reuseport, bool nonblock);
		ssize_t UdpListen(const std::string_view& listenHostPort, bool reuseaddress, bool reuseport, bool nonblock);
		void UdpClose();
		virtual void OnUdpData(fd_t fd, const inet::ssl_t& ssl, const std::weak_ptr<inet::cpoll>& poll) = 0;
		virtual std::shared_ptr<inet::cudpserver> UdpSelf() = 0;
		virtual inline std::shared_ptr<cserver> GetInstance() { return std::dynamic_pointer_cast<cserver>(UdpSelf()); }
	private:
		virtual void OnAccept(fd_t, uint, std::weak_ptr<cpoll> poll) override;
		virtual inline void OnAcceptSSL(fd_t, uint, std::weak_ptr<cpoll> poll) override { ; }
	private:
		fd_t srvFd{ -1 };
	};
}
