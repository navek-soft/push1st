#pragma once
#include "cinet.h"
#include "cepoll.h"

namespace inet {
	class csocket {
	public:
		csocket(fd_t so, const sockaddr_storage& sa, const inet::ssl_t& ssl, const std::weak_ptr<inet::cpoll>& poll) :
			fdSocket{ so }, fdSsl{ ssl }, fdPoll{ poll }
		{
			if (ssl) { write_fn = &csocket::write_ssl; read_fn = &csocket::read_ssl; }
			bcopy(&sa, &fdSa, sizeof(sockaddr_storage));
		}
		csocket(fd_t so,const inet::ssl_t& ssl) :
			fdSocket{ so }, fdSsl{ ssl }
		{
			if (ssl) { write_fn = &csocket::write_ssl; read_fn = &csocket::read_ssl; }
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

		inline csocket& operator = (const csocket&& so);
		inline csocket& operator = (fd_t so);
		virtual ~csocket() { ; }
		virtual inline void OnSocketConnect() { ; }
		virtual inline void OnSocketRecv() { ; }
		virtual inline void OnSocketSend() { ; }
		virtual inline void OnSocketError(ssize_t err) { ; }
		inline operator bool() { return fdSocket > 0; }
	public:
		inline void SocketUpdateEvents(uint events);
		inline ssize_t SocketSend(const void* data, size_t length, size_t& nwrite, uint flags) const { return (this->*write_fn)(data, length, nwrite, flags); }
		inline ssize_t SocketRecv(void* data, size_t length, size_t& nread, uint flags) const { return (this->*read_fn)(data, length, nread, flags); }
		inline ssize_t SocketSend(const sockaddr_storage& sa, const void* data, size_t length, size_t& nwrite, uint flags) const { return (this->*write_to_fn)(sa, data, length, nwrite, flags); }
		inline ssize_t SocketRecv(sockaddr_storage& sa, void* data, size_t length, size_t& nread, uint flags) const { return (this->*read_from_fn)(sa, data, length, nread, flags); }
		inline int Fd() const { return (int)fdSocket; }
		inline auto Poll() const { return fdPoll.lock(); }
		inline const sockaddr_storage& Address() const { return fdSa; }
		int SocketClose() const;
		inline std::string GetAddress() const { return inet::GetIp(fdSa); }
		inline uint32_t GetIp4() const { return inet::GetIp4(fdSa); }
		inline uint8_t* GetIp6() const { return inet::GetIp6(fdSa); }
		inline int GetAF() const { return inet::GetAF(fdSa); }
		inline uint16_t GetPort() const { return inet::GetPort(fdSa); }
		inline ssize_t GetErrorNo() const { return inet::GetErrorNo(fdSocket); }
		inline ssize_t SetKeepAlive(bool enable, int count, int idle_sec, int intrvl_sec) const { return inet::SetKeepAlive(fdSocket, enable, count, idle_sec, intrvl_sec); }
		inline ssize_t SetNonBlock(bool nonblock) const { return inet::SetNonBlock(fdSocket, nonblock); }
		inline ssize_t SetRecvTimeout(std::time_t milliseconds) const { return inet::SetRecvTimeout(fdSocket, (size_t)milliseconds); }
		inline ssize_t SetSendTimeout(std::time_t milliseconds) const { return inet::SetSendTimeout(fdSocket, (size_t)milliseconds); }
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

	inline csocket& csocket::operator = (const csocket&& so)
	{
		fdSocket = so.fdSocket;
		fdSsl = so.fdSsl;
		fdPoll = so.fdPoll;
		so.fdSocket = -1;
		so.fdSsl.reset();
		bcopy(&so.fdSa, &fdSa, sizeof(sockaddr_storage));
		if (fdSsl) { write_fn = &csocket::write_ssl; read_fn = &csocket::read_ssl; }
		return *this;
	}

	inline csocket& csocket::operator = (fd_t so)
	{
		fdSocket = so;
		fdSsl.reset();
		fdPoll.reset();
		bzero(&fdSa, sizeof(sockaddr_storage));
		if (fdSsl) { write_fn = &csocket::write_ssl; read_fn = &csocket::read_ssl; }
		return *this;
	}

	inline void csocket::SocketUpdateEvents(uint events) {
		if (auto&& poll{ fdPoll.lock() }; poll and fdSocket > 0) {
			if (auto res = poll->PollUpdate(fdSocket, events); res == 0) {
				return;
			}
			else {
				OnSocketError(res);
			}
		}
		else {
			OnSocketError(-EBADFD);
		}
	}

	using socket_t = std::shared_ptr<csocket>;
}
