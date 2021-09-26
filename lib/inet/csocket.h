#pragma once
#include "cinet.h"
#include "cepoll.h"

namespace inet {
	class csocket {
	public:
		csocket(fd_t so, const sockaddr_storage& sa, const std::shared_ptr<struct ssl_st>& ssl, const std::weak_ptr<inet::cpoll>& poll) :
			fdSocket{ so }, fdSsl{ ssl }, fdPoll{ poll }
		{
			if (ssl) { write_fn = &csocket::write_ssl; read_fn = &csocket::read_ssl; }
			bcopy(&sa, &fdSa, sizeof(sockaddr_storage));
		}
		csocket(const csocket&) = delete;
		csocket(const csocket&& so) : 
			fdSocket{ so.fdSocket }, fdSsl{ so.fdSsl }, fdPoll{ so.fdPoll } 
		{
			so.fdSocket = -1;
			so.fdSsl.reset();
			bcopy(&so.fdSa, &fdSa, sizeof(sockaddr_storage));
			if (fdSsl) { write_fn = &csocket::write_ssl; read_fn = &csocket::read_ssl; }
		}
		virtual ~csocket() { printf("%s\n", __PRETTY_FUNCTION__); }
		virtual inline void OnSocketConnect() { ; }
		virtual inline void OnSocketRecv() { ; }
		virtual inline void OnSocketSend() { ; }
		virtual inline void OnSocketError(ssize_t err) { ; }
	public:
		inline ssize_t SocketSend(const void* data, size_t length, size_t& nwrite, uint flags) const { return (this->*write_fn)(data, length, nwrite, flags); }
		inline ssize_t SocketRecv(void* data, size_t length, size_t& nread, uint flags) const { return (this->*read_fn)(data, length, nread, flags); }
		inline ssize_t SocketSend(const sockaddr_storage& sa, const void* data, size_t length, size_t& nwrite, uint flags) const { return (this->*write_to_fn)(sa, data, length, nwrite, flags); }
		inline ssize_t SocketRecv(sockaddr_storage& sa, void* data, size_t length, size_t& nread, uint flags) const { return (this->*read_from_fn)(sa, data, length, nread, flags); }
		inline int Fd() const { return (int)fdSocket; }
		inline const sockaddr_storage& Address() const { return fdSa; }
		int SocketClose();
		inline std::string GetAddress() { return inet::GetIp(fdSa); }
		inline uint32_t GetIp4() { return inet::GetIp4(fdSa); }
		inline uint8_t* GetIp6() { return inet::GetIp6(fdSa); }
		inline int GetAF() { return inet::GetAF(fdSa); }
		inline uint16_t GetPort() { return inet::GetPort(fdSa); }
		inline ssize_t GetErrorNo() { return inet::GetErrorNo(fdSocket); }
		inline ssize_t SetKeepAlive(bool enable, int count, int idle_sec, int intrvl_sec) { return inet::SetKeepAlive(fdSocket, enable, count, idle_sec, intrvl_sec); }
		inline ssize_t SetNonBlock(bool nonblock) { return inet::SetNonBlock(fdSocket, nonblock); }
		inline ssize_t SetRecvTimeout(std::time_t milliseconds) { return inet::SetRecvTimeout(fdSocket, (size_t)milliseconds); }
		inline ssize_t SetSendTimeout(std::time_t milliseconds) { return inet::SetSendTimeout(fdSocket, (size_t)milliseconds); }
	private:
		mutable fd_t fdSocket;
		mutable std::shared_ptr<struct ssl_st> fdSsl;
		sockaddr_storage fdSa;
		std::weak_ptr<inet::cpoll> fdPoll;

		inline struct ssl_st* ssl() const { return fdSsl.get(); }

		ssize_t write_nossl(const void* data, size_t length, size_t& nwrite, uint flags) const;
		ssize_t write_ssl(const void* data, size_t length, size_t& nwrite, uint flags) const;
		ssize_t read_nossl(void* data, size_t length, size_t& nread, uint flags) const;
		ssize_t read_ssl(void* data, size_t length, size_t& nread, uint flags) const;
		ssize_t write_to_nossl(const sockaddr_storage& sa, const void* data, size_t length, size_t& nwrite, uint flags) const;
		ssize_t write_to_ssl(const sockaddr_storage& sa, const void* data, size_t length, size_t& nwrite, uint flags) const;
		ssize_t read_from_nossl(sockaddr_storage& sa, void* data, size_t length, size_t& nread, uint flags) const;
		ssize_t read_from_ssl(sockaddr_storage& sa, void* data, size_t length, size_t& nread, uint flags) const;

		ssize_t(csocket::* write_fn)(const void* data, size_t length, size_t& nwrite, uint flags) const { &csocket::write_nossl };
		ssize_t(csocket::* read_fn)(void* data, size_t length, size_t& nread, uint flags) const { &csocket::read_nossl };
		ssize_t(csocket::* write_to_fn)(const sockaddr_storage& sa, const void* data, size_t length, size_t& nwrite, uint flags) const { &csocket::write_to_nossl };
		ssize_t(csocket::* read_from_fn)(sockaddr_storage& sa, void* data, size_t length, size_t& nread, uint flags) const { &csocket::read_from_nossl };
	};

	using socket_t = std::shared_ptr<csocket>;
}
