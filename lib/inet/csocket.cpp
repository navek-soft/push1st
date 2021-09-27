#include "csocket.h"
#include "cepoll.h"
#include <sys/socket.h>
#include <unistd.h>

extern "C" {
#include <openssl/ssl.h>
#include <openssl/err.h>
}

using namespace inet;

int csocket::SocketClose() const {
	if (int fd{ fdSocket }; fd > 0) {
		if (auto&& poll{ fdPoll.lock() }; poll) {
			poll->PollDelete(fdSocket);
		}
		else {
			::close(fdSocket);
			fdSocket = -1;
		}
		fdSsl.reset();
		return fd;
	}
	return -1;
}

ssize_t csocket::write_to_nossl(const sockaddr_storage& sa, const void* data, size_t length, size_t& nwrited, uint flags) const {
	ssize_t nwrite{ 0 };
	uint8_t* buffer{ (uint8_t*)data };
	nwrited = 0;
	while (length) {
		if ((nwrite = ::sendto((int)fdSocket, buffer, length, flags | MSG_NOSIGNAL, (const struct sockaddr*)&sa, sizeof(sa))) > 0) {
			length -= nwrite; nwrited += nwrite; buffer += nwrite;
			continue;
		}
		else if (nwrite == -1 and errno == EAGAIN) {
			continue;
		}
		break;
	}
	return nwrite > 0 && !length ? 0 : -errno;
}

ssize_t csocket::write_to_ssl(const sockaddr_storage& sa, const void* data, size_t length, size_t& nwrited, uint flags) const {
	return -ESOCKTNOSUPPORT;
}

ssize_t csocket::read_from_nossl(sockaddr_storage& sa, void* data, size_t length, size_t& nreaded, uint flags) const {
	ssize_t nread{ 0 };
	uint8_t* buffer{ (uint8_t*)data };
	nreaded = 0;
	socklen_t salen{ sizeof(sockaddr_storage) };
	while (length) {
		if ((nread = ::recvfrom((int)fdSocket, buffer, length, flags | MSG_NOSIGNAL, (sockaddr*)&sa, &salen)) > 0) {
			length -= nread; nreaded += nread; buffer += nread;
			continue;
		}
		else if (nread == 0) {
			return -ECONNABORTED;
		}
		break;
	}
	return nread > 0 && !length ? 0 : ((flags & MSG_DONTWAIT) && errno == EAGAIN) ? 0 : -errno;
}

ssize_t csocket::read_from_ssl(sockaddr_storage& sa, void* data, size_t length, size_t& nread, uint flags) const {
	return -ESOCKTNOSUPPORT;
}


ssize_t csocket::write_nossl(const void* data, size_t length, size_t& nwrited, uint flags) const {
	ssize_t nwrite{ 0 };
	uint8_t* buffer{ (uint8_t*)data };
	nwrited = 0;
	while (length) {
		if ((nwrite = ::send((int)fdSocket, buffer, length, flags | MSG_NOSIGNAL)) > 0) {
			length -= nwrite; nwrited += nwrite; buffer += nwrite;
			continue;
		}
		else if (nwrite == -1 and errno == EAGAIN) {
			continue;
		}
		break;
	}
	return nwrite > 0 && !length ? 0 : -errno;
}

ssize_t csocket::write_ssl(const void* data, size_t length, size_t& nwrited, uint flags) const {
	ssize_t nwrite{ 0 };
	uint8_t* buffer{ (uint8_t*)data };
	nwrited = 0;
	while (length) {
		if ((nwrite = SSL_write(ssl(), buffer, (int)length)) > 0) {
			length -= nwrite; nwrited += nwrite; buffer += nwrite;
			continue;
		}
		else if (nwrite == -1 and SSL_get_error(ssl(), (int)nwrite) == SSL_ERROR_WANT_WRITE) {
			continue;
		}
		break;
	}
	switch (int err = SSL_get_error(ssl(), (int)nwrite); err) {
	case SSL_ERROR_NONE:
		return 0;
	case SSL_ERROR_WANT_READ:
	case SSL_ERROR_WANT_WRITE:
		return -EAGAIN;
		break;
	case SSL_ERROR_ZERO_RETURN:
		return -ECONNABORTED;
		break;
	default:
		break;
	}
	return -EBADFD;
}

ssize_t csocket::read_nossl(void* data, size_t length, size_t& nreaded, uint flags) const {
	ssize_t nread{ 0 };
	uint8_t* buffer{ (uint8_t*)data };
	nreaded = 0;
	while (length) {
		if ((nread = ::recv((int)fdSocket, buffer, length, flags | MSG_NOSIGNAL)) > 0) {
			length -= nread; nreaded += nread; buffer += nread;
			continue;
		}
		else if (nread == 0) {
			return -ECONNABORTED;
		}
		break;
	}
	return nread > 0 && !length ? 0 : ((flags & MSG_DONTWAIT) && errno == EAGAIN) ? 0 : -errno;
}

ssize_t csocket::read_ssl(void* data, size_t length, size_t& nreaded, uint flags) const {

	ssize_t nread{ 0 };
	uint8_t* buffer{ (uint8_t*)data };
	nreaded = 0;
	while (length) {
		if ((nread = SSL_read(ssl(), buffer, (int)length)) > 0) {
			length -= nread; nreaded += nread; buffer += nread;
			if (!(flags & MSG_DONTWAIT)) continue;
			return 0;
		}
		break;
	}
	switch (int err = SSL_get_error(ssl(), (int)nread); err) {
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