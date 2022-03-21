#include "cclient.h"
#include <unistd.h>

extern "C" {
#include <openssl/ssl.h>
#include <openssl/err.h>
}

ssize_t cclient::send_nossl(const void* data, size_t length, uint flags, const sockaddr_storage* sa) const {
	return ::sendto(cliFd, data, length, flags | MSG_NOSIGNAL, (const struct sockaddr*)&sa, sizeof(sa));
}

ssize_t cclient::send_ssl(const void* data, size_t length, uint flags, const  sockaddr_storage* sa) const {
	ssize_t nwrite{ 0 };
	
	if ((nwrite = SSL_write(cliSsl.get(), data, (int)length)) > 0) {
		return nwrite;
	}

	switch (int err = SSL_get_error(cliSsl.get(), (int)nwrite); err) {
	case SSL_ERROR_NONE:
		return 0;
	case SSL_ERROR_WANT_READ:
	case SSL_ERROR_WANT_WRITE:
		return (flags & MSG_DONTWAIT) ? 0 : -EAGAIN;
		break;
	case SSL_ERROR_ZERO_RETURN:
		return -ECONNABORTED;
		break;
	default:
		break;
	}
	return -EBADFD;
}

ssize_t cclient::recv_nossl(void* data, size_t length, uint flags, sockaddr_storage* sa) const {
	socklen_t saLen{ sizeof(sockaddr_storage) };
	return ::recvfrom(cliFd, data, length, flags | MSG_NOSIGNAL, (struct sockaddr*)&sa, &sa);
}

ssize_t cclient::recv_ssl(void* data, size_t length, uint flags, sockaddr_storage* sa) const {
	ssize_t nread{ 0 };

	if ((nread = SSL_read(cliSsl.get(), data, (int)length)) > 0) {
		return nread;
	}

	switch (int err = SSL_get_error(cliSsl.get(), (int)nread); err) {
	case SSL_ERROR_NONE:
		return 0;
	case SSL_ERROR_WANT_READ:
	case SSL_ERROR_WANT_WRITE:
		return (flags & MSG_DONTWAIT) ? 0 : -EAGAIN;
		break;
	case SSL_ERROR_ZERO_RETURN:
		return -ECONNABORTED;
		break;
	default:
		break;
	}
	return -EBADFD;
}

void cclient::Close() {
	if (cliFd > 0) {
		::close(cliFd);
		cliFd = -1;
	}
}

cclient::cclient(int fd, std::unique_ptr<sockaddr_storage>&& sa, const std::shared_ptr<cpoll>& poll, const ssl_t& ssl) :
	cliFd{ fd }, cliSa{ std::move(sa) }, cliSsl{ ssl }
{
	if (cliSsl) { send_fn = &cclient::send_ssl; recv_fn = &cclient::recv_ssl; }
}

cclient::cclient(cclient&& cli) : 
	cliFd{ cli.cliFd }, cliSa{ std::move(cli.cliSa) }
{
	cli.cliFd = -1;
}

cclient::~cclient() {
	Close();
}