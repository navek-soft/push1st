#include "cinet.h"
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <unistd.h>
#include <fcntl.h>
#include <regex>
#include <cstring>
#include <sys/un.h>


extern "C" {
#include <openssl/ssl.h>
#include <openssl/err.h>
}

namespace inet::ssl {
	static std::shared_ptr<SSL_CTX> gloContext;
}

int inet::Close(int& sock) {
	if (fd_t fd{ sock }; fd > 0) { shutdown(fd, SHUT_RDWR); ::close(fd); sock = -1; return fd; }
	return -1;
}


ssize_t inet::GetAddress(fd_t fd, sockaddr_storage& sa) {
	bzero(&sa, sizeof(sa));
	socklen_t len = sizeof(sockaddr_storage);
	return getsockname((int)fd, (struct sockaddr*)&sa, &len) == 0 ? 0 : -errno;
}

std::string inet::GetIp(const sockaddr_storage& sa) {
	auto&& buffer{ (char*)alloca(256) };
	if (sa.ss_family == AF_INET) {
		inet_ntop(sa.ss_family, &((sockaddr_in&)sa).sin_addr, buffer, 256);
		return buffer;
	}
	else if (sa.ss_family == AF_INET6) {
		inet_ntop(sa.ss_family, &((sockaddr_in6&)sa).sin6_addr, buffer, 256);
		return buffer;
	}
	return {};
}

uint32_t inet::GetIp4(const sockaddr_storage& sa) {
	if (sa.ss_family == AF_INET) {
		return ((sockaddr_in&)sa).sin_addr.s_addr;
	}
	else if (sa.ss_family == AF_INET6) {
		return ((sockaddr_in6&)sa).sin6_addr.__in6_u.__u6_addr32[3];
	}
	return 0;
}

uint8_t* inet::GetIp6(const sockaddr_storage& sa) {
	if (sa.ss_family == AF_INET6) {
		return ((sockaddr_in6&)sa).sin6_addr.__in6_u.__u6_addr8;
	}
	return nullptr;
}

uint16_t inet::GetPort(const sockaddr_storage& sa) {
	if (sa.ss_family == AF_INET) {
		return ((sockaddr_in&)sa).sin_port;
	}
	else if (sa.ss_family == AF_INET6) {
		return ((sockaddr_in6&)sa).sin6_port;
	}
	return 0;
}

ssize_t inet::GetErrorNo(fd_t fd) {
	int soerror{ 0 };
	if (socklen_t so_len{ sizeof(soerror) }; getsockopt((int)fd, SOL_SOCKET, SO_ERROR, &soerror, &so_len) == 0) {
		soerror == 0 and getsockopt((int)fd, SOL_SOCKET, SO_LINGER, &soerror, &so_len);
		return soerror == 0 ? 0 : -soerror;
	}
	return errno == EAGAIN ? 0 : -errno;
//	return !soerror? -soerror : ((!errno || errno == EAGAIN) ? 0 : -errno);
}

ssize_t inet::SetKeepAlive(fd_t fd, bool enable, int count, int idle_sec, int intrvl_sec) {
	if (int flagKeepAlive = enable ? 1 : 0; setsockopt((int)fd, SOL_SOCKET, SO_KEEPALIVE, &flagKeepAlive, sizeof(flagKeepAlive)) == 0) {
		if (count >= 0 && setsockopt((int)fd, IPPROTO_TCP, TCP_KEEPCNT, &count, sizeof(count)) != 0) {
			return -errno;
		}
		if (idle_sec >= 0 && setsockopt((int)fd, IPPROTO_TCP, TCP_KEEPIDLE, &idle_sec, sizeof(idle_sec)) != 0) {
			return -errno;
		}
		if (intrvl_sec >= 0 && setsockopt((int)fd, IPPROTO_TCP, TCP_KEEPINTVL, &intrvl_sec, sizeof(intrvl_sec)) != 0) {
			return -errno;
		}
		return 0;
	}
	return -errno;
}

ssize_t inet::SetNonBlock(fd_t fd, bool nonblock) {
	int flags = fcntl((int)fd, F_GETFL, 0);
	return fcntl((int)fd, F_SETFL, nonblock ? (flags | O_NONBLOCK) : (flags & (~O_NONBLOCK))) == 0 ? 0 : -errno;
}

ssize_t inet::SetRecvTimeout(fd_t fd, size_t milliseconds) {
	struct timeval tv;
	tv.tv_sec = milliseconds / 1000;
	tv.tv_usec = (milliseconds % 1000) * 1000;
	return setsockopt((int)fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv) == 0 ? 0 : -errno;
}

ssize_t inet::SetUdpCork(fd_t fd, bool enable) {
	int cork = enable ? 1 : 0;
	return setsockopt((int)fd, IPPROTO_UDP, UDP_CORK, &cork, sizeof(cork)) == 0 ? 0 : -errno;
}

ssize_t inet::SetTcpCork(fd_t fd, bool enable) {
	int cork = enable ? 1 : 0;
	return setsockopt((int)fd, IPPROTO_TCP, TCP_CORK, &cork, sizeof(cork)) == 0 ? 0 : -errno;
}

ssize_t inet::SetTcpNoDelay(fd_t fd, bool enable) {
	int nodelay = enable ? 1 : 0;
	return setsockopt((int)fd, IPPROTO_TCP, TCP_NODELAY, &nodelay, sizeof(nodelay)) == 0 ? 0 : -errno;
}


ssize_t inet::SetSendTimeout(fd_t fd, size_t milliseconds) {
	struct timeval tv;
	tv.tv_sec = milliseconds / 1000;
	tv.tv_usec = (milliseconds % 1000) * 1000;
	return setsockopt((int)fd, SOL_SOCKET, SO_SNDTIMEO, (const char*)&tv, sizeof tv) == 0 ? 0 : -errno;
}

ssize_t inet::TcpAccept(fd_t fd, fd_t& cli, sockaddr_storage& sa, bool nonblock) {
	socklen_t len{ sizeof(sockaddr_storage) };
	if (cli = accept4((int)fd, (sockaddr*)&sa, &len, SOCK_CLOEXEC | (nonblock ? SOCK_NONBLOCK : 0)); cli > 0) {
		return 0;
	}
	return -errno;
}

ssize_t inet::UdpConnect(fd_t& fd, bool nonblock, int af) {
	fd = -1;
	if (fd = ::socket(af, SOCK_CLOEXEC | SOCK_DGRAM | (nonblock ? SOCK_NONBLOCK : 0), IPPROTO_UDP); fd <= 0) { return -errno; }

	return 0;
}

ssize_t inet::UdpConnect(fd_t& fd, const sockaddr_storage& sa, bool nonblock) {
	ssize_t res{ 0 };
	fd = -1;
	if (fd = ::socket(sa.ss_family, SOCK_CLOEXEC | SOCK_DGRAM | (nonblock ? SOCK_NONBLOCK : 0), IPPROTO_UDP); fd <= 0) { return -errno; }
	if (::connect((int)fd, (sockaddr*)&sa, sizeof(sockaddr_storage)) != 0) { res = -errno; ::close((int)fd); fd = -1; return res; }

	return 0;
}

ssize_t inet::TcpConnect(fd_t& fd, const sockaddr_storage& sa, bool nonblock, std::time_t conntimeout) {
	ssize_t res{ 0 };
	fd = -1;
	if (fd = ::socket(sa.ss_family, SOCK_CLOEXEC | SOCK_STREAM | (nonblock ? SOCK_NONBLOCK : 0), 0); fd > 0) {
		if (conntimeout) { inet::SetSendTimeout(fd, conntimeout); }
		if (::connect((int)fd, (sockaddr*)&sa, sizeof(sockaddr_storage)) == 0) {
			return 0;
		}
		if (res = -errno; errno == EINPROGRESS) {
			return res;
		}
		::close((int)fd);
		fd = -1;
	}
	return res;
}

ssize_t inet::SslCreateClientContext(ssl_ctx_t& sslCtx) {
	if (sslCtx = std::shared_ptr<SSL_CTX>{ SSL_CTX_new(TLS_client_method()),[](SSL_CTX* ctx) { SSL_CTX_free(ctx); } }; sslCtx) {
		return 0;
	}
	return -EINVAL;
}

ssize_t inet::SslConnect(fd_t& fd, const sockaddr_storage& sa, bool nonblock, std::time_t conntimeout, const inet::ssl_ctx_t& ctx, inet::ssl_t& ssl) {
	ssize_t res{ 0 };
	fd = -1;

	if (ctx) {
		if (ssl = std::shared_ptr<SSL>(SSL_new(ctx.get()), [](SSL* ssl) { SSL_free(ssl); }); !ssl) {
			return -EINVAL;
		}
	}
	else {
		return -EINVAL;
	}

	if (fd = ::socket(sa.ss_family, SOCK_CLOEXEC | SOCK_STREAM | (nonblock ? SOCK_NONBLOCK : 0), 0); fd > 0) {
		if (conntimeout) { inet::SetSendTimeout(fd, conntimeout); }
		
		SSL_set_fd(ssl.get(), (int)fd);

		if (::connect((int)fd, (sockaddr*)&sa, sizeof(sockaddr_storage)) == 0) {
			return 0;
		}
		if (res = -errno; errno == EINPROGRESS) {
			return res;
		}
		ssl.reset();
		::close((int)fd);
		fd = -1;
	}
	return res;
}

ssize_t inet::TcpServer(int& fd, const sockaddr_storage& sa, bool reuseaddress, bool reuseport, bool nonblock, int maxlisten) {
	ssize_t res{ 0 };
	fd = -1;
	if (fd = ::socket(sa.ss_family, SOCK_CLOEXEC | SOCK_STREAM | (nonblock ? SOCK_NONBLOCK : 0), 0); fd <= 0) { return -errno; }
	if (int nvalue = 1; reuseaddress && setsockopt((int)fd, SOL_SOCKET, SO_REUSEADDR, &nvalue, sizeof(nvalue)) != 0) { res = -errno; ::close((int)fd); fd = -1; return res; }
	if (int nvalue = 1; reuseport && setsockopt((int)fd, SOL_SOCKET, SO_REUSEPORT, &nvalue, sizeof(nvalue)) != 0) { res = -errno; ::close((int)fd); fd = -1; return res; }
	if (::bind((int)fd, (sockaddr*)&sa, sizeof(sockaddr_storage)) != 0) { res = -errno; ::close((int)fd); fd = -1; return res; }
	if (::listen((int)fd, maxlisten) != 0) { res = -errno; ::close((int)fd); fd = -1; return res; }

	return 0;
}

ssize_t inet::UdpServer(int& fd, const sockaddr_storage& sa, bool reuseaddress, bool reuseport, bool nonblock) {
	ssize_t res{ 0 };
	fd = -1;
	if (fd = ::socket(sa.ss_family, SOCK_CLOEXEC | SOCK_DGRAM | (nonblock ? SOCK_NONBLOCK : 0), IPPROTO_UDP); fd <= 0) { return -errno; }
	//if (int nvalue = 1; reuseaddress && setsockopt((int)fd, SOL_SOCKET, SO_REUSEADDR, &nvalue, sizeof(nvalue)) != 0) { res = -errno; ::close((int)fd); fd = -1; return res; }
	//if (int nvalue = 1; reuseport && setsockopt((int)fd, SOL_SOCKET, SO_REUSEPORT, &nvalue, sizeof(nvalue)) != 0) { res = -errno; ::close((int)fd); fd = -1; return res; }
	if (::bind((int)fd, (sockaddr*)&sa, sizeof(sockaddr_storage)) != 0) { res = -errno; ::close((int)fd); fd = -1; return res; }

	return 0;
}

ssize_t inet::UnixServer(int& fd, const sockaddr_storage& sa, bool nonblock, int maxlisten) {
	ssize_t res{ 0 };
	fd = -1;
	if (fd = ::socket(sa.ss_family, SOCK_CLOEXEC | SOCK_STREAM | (nonblock ? SOCK_NONBLOCK : 0), 0); fd <= 0) { return -errno; }

	unlink(((const struct sockaddr_un&)sa).sun_path);

	if (::bind((int)fd, (sockaddr*)&sa, sizeof(sockaddr_un)) != 0) {
		res = -errno; ::close((int)fd); fd = -1;
		return res;
	}
	if (::listen((int)fd, maxlisten) != 0) { res = -errno; ::close((int)fd); fd = -1; return res; }

	return 0;
}

ssize_t inet::UnixAccept(fd_t fd, fd_t& cli, sockaddr_storage& sa, bool nonblock) {
	socklen_t len{ sizeof(sockaddr_storage) };
	if (cli = accept4((int)fd, (sockaddr*)&sa, &len, SOCK_CLOEXEC | (nonblock ? SOCK_NONBLOCK : 0)); cli > 0) {
		return 0;
	}
	return -errno;
}


ssize_t inet::GetSockAddr(sockaddr_storage& sa, const std::string_view& strHostPort, const std::string_view& strPort, int defaultAF) {
	static const std::regex reHostPort(R"((?:(\d+\.[\d.]+)|([\da-f]+:[\da-f:]+)|([^:]*))\:?(\d*))");

	bzero(&sa, sizeof(sa));

	try {
		if (defaultAF != AF_UNIX) {
			std::cmatch match;
			if (std::regex_match(strHostPort.begin(), strHostPort.end(), match, reHostPort)) {
				if (match.length(1)) { // ip4 address found
					if (auto res = inet_pton(AF_INET, match.str(1).c_str(), (void*)(&(((sockaddr_in*)&sa)->sin_addr.s_addr))); res > 0) {
						sa.ss_family = AF_INET;
					}
					else {
						return -errno;
					}
				}
				else if (match.length(2)) { // ip6 address found
					if (auto res = inet_pton(AF_INET6, match.str(2).c_str(), (void*)((sockaddr_in6*)&sa)->sin6_addr.__in6_u.__u6_addr8); res > 0) {
						sa.ss_family = AF_INET6;
					}
					else {
						return -errno;
					}
				}
				else if (auto hostname{ match.str(3) }; !hostname.empty() && hostname != "*") { /* get addr by hostname */
					sa.ss_family = (sa_family_t)defaultAF;
					if (/* is hostname */ auto haddr = gethostbyname2(hostname.c_str(), sa.ss_family); haddr) {
						if (haddr->h_addrtype == AF_INET) {
							sa.ss_family = AF_INET;
							memcpy(&(((sockaddr_in*)&sa)->sin_addr.s_addr), haddr->h_addr, haddr->h_length);
						}
						else if (haddr->h_addrtype == AF_INET6) {
							sa.ss_family = AF_INET6;
							memcpy(((sockaddr_in6*)&sa)->sin6_addr.__in6_u.__u6_addr8, haddr->h_addr, haddr->h_length);
						}
						else {
							return -EAFNOSUPPORT;
						}
					}
					else {
						return -h_errno;
					}
				}
				else /* bind to all interfaces */ {
					((sockaddr_in*)&sa)->sin_addr.s_addr = INADDR_ANY;
					((sockaddr_in*)&sa)->sin_family = (sa_family_t)defaultAF;
				}
				if (auto saPort{ match.str(4) }; !saPort.empty()) {
					if (sa.ss_family == AF_INET6) {
						((sockaddr_in6*)&sa)->sin6_port = htons((uint16_t)std::stoi(saPort));
					}
					else {
						((sockaddr_in*)&sa)->sin_port = htons((uint16_t)std::stoi(saPort));
					}
				}
				else {
					if (sa.ss_family == AF_INET6) {
						((sockaddr_in6*)&sa)->sin6_port = htons((uint16_t)std::stoi(std::string{ strPort }));
					}
					else {
						((sockaddr_in*)&sa)->sin_port = htons((uint16_t)std::stoi(std::string{ strPort }));
					}
				}
			}
			else {
				return -EPFNOSUPPORT;
			}
		}
		else {
			((struct sockaddr_un*)&sa)->sun_family = AF_UNIX;
			strncpy(((struct sockaddr_un*)&sa)->sun_path, strHostPort.data(), std::min(108ul, strHostPort.length()));
		}
	}
	catch (std::regex_error& e) {
		// Syntax error in the regular expression
		return -EINVAL;
	}

	return 0;
}

ssize_t inet::SslSetGlobalContext(const std::shared_ptr<SSL_CTX>& sslCtx) {
	inet::ssl::gloContext = std::move(sslCtx);
	return 0;
}

std::shared_ptr<SSL_CTX> inet::SslGetGlobalContext() {
	return inet::ssl::gloContext;
}


ssize_t inet::SslAcceptConnection(ssize_t cli, const std::shared_ptr<SSL_CTX>& sslCtx, std::shared_ptr<SSL>& sslSocket) {
	if (sslSocket = std::shared_ptr<SSL>{ SSL_new(sslCtx.get()),[](SSL* ssl) { SSL_shutdown(ssl); SSL_free(ssl); } }; sslSocket) {
		if (SSL_set_fd(sslSocket.get(), (int)cli) == 1 and SSL_accept(sslSocket.get()) == 1) {
			return 0;
		}
		ERR_print_errors_fp(stderr);
		sslSocket.reset();
		return -EPROTO;
	}
	return -ENOMEM;
}


ssize_t inet::SslCreateSelfSignedContext(std::shared_ptr<SSL_CTX>& sslCtx) {
	SSL_library_init();

	/* Load the error strings for good error reporting */
	SSL_load_error_strings();

	/* Load BIO error strings. */
	ERR_load_BIO_strings();

	/* Load all available encryption algorithms. */
	OpenSSL_add_all_algorithms();

	if (sslCtx = std::shared_ptr<SSL_CTX>{ SSL_CTX_new(SSLv23_server_method()),[](SSL_CTX* ctx) { SSL_CTX_free(ctx); } }; !sslCtx) {
		perror("INET:SSL (create self signed context)");
		ERR_print_errors_fp(stderr);
		return -1;
	}

	EVP_PKEY* pkey = NULL;
	X509* x509 = X509_new();

	{
		EVP_PKEY_CTX* pctx = EVP_PKEY_CTX_new_id(EVP_PKEY_RSA, NULL);
		EVP_PKEY_keygen_init(pctx);
		EVP_PKEY_CTX_set_rsa_keygen_bits(pctx, 2048);
		EVP_PKEY_keygen(pctx, &pkey);
	}

	{
		X509_set_version(x509, 2);
		ASN1_INTEGER_set(X509_get_serialNumber(x509), 0);
		X509_gmtime_adj(X509_get_notBefore(x509), 0);
		X509_gmtime_adj(X509_get_notAfter(x509), (long)60 * 60 * 24 * 365);
		X509_set_pubkey(x509, pkey);

		X509_NAME* name = X509_get_subject_name(x509);
		X509_NAME_add_entry_by_txt(name, "C", MBSTRING_ASC, (const unsigned char*)"BY", -1, -1, 0);
		X509_NAME_add_entry_by_txt(name, "CN", MBSTRING_ASC, (const unsigned char*)"NavekSoft", -1, -1, 0);
		X509_set_issuer_name(x509, name);
		X509_sign(x509, pkey, EVP_md5());
	}

	SSL_CTX_use_certificate(sslCtx.get(), x509);
	SSL_CTX_set_default_passwd_cb(sslCtx.get(), [](char* buf, int size, int rwflag, void* password) -> int
		{
			strncpy(buf, (char*)(password), size);
			buf[size - 1] = '\0';
			return((int)strlen(buf));
		}
	);
	SSL_CTX_use_PrivateKey(sslCtx.get(), pkey);

	RSA* rsa = RSA_generate_key(4096, RSA_F4, NULL, NULL);
	SSL_CTX_set_tmp_rsa(sslCtx.get(), rsa);
	RSA_free(rsa);

	SSL_CTX_set_verify(sslCtx.get(), SSL_VERIFY_NONE, 0);

	return 0;
}

ssize_t inet::SslCreateContext(std::shared_ptr<SSL_CTX>& sslCtx, const std::string& certFile, const std::string& privateKeyFile, bool novalidate) {
	SSL_library_init();

	/* Load the error strings for good error reporting */
	SSL_load_error_strings();

	/* Load BIO error strings. */
	//ERR_load_BIO_strings();

	/* Load all available encryption algorithms. */
	OpenSSL_add_all_algorithms();

	if (!certFile.empty() && !privateKeyFile.empty()) {
		sslCtx = std::shared_ptr<SSL_CTX>{ SSL_CTX_new(SSLv23_server_method()),[](SSL_CTX* ctx) { SSL_CTX_free(ctx); } };
		if (!sslCtx) {
			perror("INET:SSL (create context)");
			ERR_print_errors_fp(stderr);
			return -1;
		}

		SSL_CTX_set_mode(sslCtx.get(), SSL_MODE_SEND_FALLBACK_SCSV);

		if (novalidate)
			SSL_CTX_set_ecdh_auto(sslCtx.get(), 1);

		SSL_CTX_set_verify(sslCtx.get(), SSL_VERIFY_NONE, NULL);

		/* Set the key and cert */
		if (SSL_CTX_use_certificate_chain_file(sslCtx.get(), certFile.c_str()) <= 0) {
			ERR_print_errors_fp(stderr);
			exit(EXIT_FAILURE);
		}

		if (SSL_CTX_use_PrivateKey_file(sslCtx.get(), privateKeyFile.c_str(), SSL_FILETYPE_PEM) <= 0) {
			ERR_print_errors_fp(stderr);
			exit(EXIT_FAILURE);
		}

		SSL_CTX_set_session_cache_mode(sslCtx.get(), SSL_SESS_CACHE_OFF);
		return 0;
	}
	else {
		return SslCreateSelfSignedContext(sslCtx);
	}
	return -ENOENT;
}
