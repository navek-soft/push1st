#pragma once
#include <string>
#include <memory>
#include <sys/socket.h>
#include <ctime>
#include <cstring>

struct ssl_st;
struct ssl_ctx_st;

using fd_t = int;

namespace inet {

	using ssl_t = std::shared_ptr<struct ssl_st>;
	using ssl_ctx_t = std::shared_ptr<struct ssl_ctx_st>;

	std::string GetIp(const sockaddr_storage& sa);
	uint32_t GetIp4(const sockaddr_storage& sa);
	uint8_t* GetIp6(const sockaddr_storage& sa);
	inline int GetAF(const sockaddr_storage& sa) { return sa.ss_family; }
	uint16_t GetPort(const sockaddr_storage& sa);
	ssize_t GetAddress(int fd, sockaddr_storage& sa);
	ssize_t GetSockAddr(sockaddr_storage& sa, const std::string_view& strHostPort, const std::string_view& strPort, int defaultAF);
	ssize_t GetErrorNo(fd_t fd);
	ssize_t SetKeepAlive(fd_t fd, bool enable, int count, int idle_sec, int intrvl_sec);
	ssize_t SetNonBlock(fd_t fd, bool nonblock);
	ssize_t SetRecvTimeout(fd_t fd, size_t milliseconds);
	ssize_t SetSendTimeout(fd_t fd, size_t milliseconds);
	ssize_t TcpAccept(fd_t fd, fd_t& cli, sockaddr_storage& sa, bool nonblock);
	ssize_t TcpConnect(fd_t& fd, const sockaddr_storage& sa, bool nonblock, std::time_t conntimeout = 0);
	ssize_t TcpServer(fd_t& fd, const sockaddr_storage& sa, bool reuseaddress, bool reuseport, bool nonblock, int maxlisten);
	ssize_t UdpServer(fd_t& fd, const sockaddr_storage& sa, bool reuseaddress, bool reuseport, bool nonblock, int maxlisten);
	ssize_t UnixServer(fd_t& fd, const sockaddr_storage& sa, bool nonblock, int maxlisten);
	ssize_t UnixAccept(fd_t fd, ssize_t& cli, sockaddr_storage& sa, bool nonblock);
	ssize_t SslCreateSelfSignedContext(ssl_ctx_t& sslCtx);
	ssize_t SslCreateContext(ssl_ctx_t& sslCtx, const std::string& certFile, const std::string& privateKeyFile, bool novalidate);
	ssize_t SslAcceptConnection(ssize_t cli, const ssl_ctx_t& sslCtx, ssl_t& sslCli);
	ssize_t SslSetGlobalContext(const ssl_ctx_t& sslCtx);
	ssl_ctx_t SslGetGlobalContext();
	inline const char* GetErrorStr(ssize_t err) { return std::strerror((int)(-err)); }
	int Close(int& sock);
}
