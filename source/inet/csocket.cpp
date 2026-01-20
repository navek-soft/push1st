#include <inet/cepoll.h>
#include <inet/csocket.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <cstddef>
#include <regex>

#include <asm-generic/errno.h>

extern "C" {
#include <openssl/err.h>
#include <openssl/ssl.h>
}

namespace inet {

int csocket::SocketClose() const {
    fdSsl.reset();
    if (auto&& poll {Poll()}; poll) {
        poll->PollDelete(fdSocket);
    }
    if (int fd {fdSocket}; fd > 0) {
        ::close(fdSocket);
        fdSocket = -1;
        return fd;
    }
    return -1;
}

ssize_t csocket::WriteToNoSsl(const sockaddr_storage& sa, const void* data, size_t length, size_t& nwrited, uint flags) const {
    ssize_t nwrite {0};
    uint8_t* buffer {(uint8_t*)data};
    nwrited = 0;
    while (length) {
        if ((nwrite = ::sendto((int)fdSocket, buffer, length, flags | MSG_NOSIGNAL, (const struct sockaddr*)&sa, sizeof(sa))) > 0) {
            length -= nwrite;
            nwrited += nwrite;
            buffer += nwrite;
            continue;
        } else if (nwrite == -1 and errno == EAGAIN) {
            continue;
        }
        break;
    }
    return nwrite > 0 && !length ? 0 : -errno;
}

ssize_t csocket::WriteToSsl([[maybe_unused]] const sockaddr_storage& sa, [[maybe_unused]] const void* data, [[maybe_unused]] size_t length, [[maybe_unused]] size_t& nwrited, [[maybe_unused]] uint flags) const {// NOLINT (static)
    return -ESOCKTNOSUPPORT;
}

ssize_t csocket::ReadFromNoSsl(sockaddr_storage& sa, void* data, size_t length, size_t& nreaded, uint flags) const {
    ssize_t nread {0};
    uint8_t* buffer {(uint8_t*)data};
    nreaded = 0;
    socklen_t salen {sizeof(sockaddr_storage)};
    while (length) {
        if ((nread = ::recvfrom((int)fdSocket, buffer, length, flags | MSG_NOSIGNAL, (sockaddr*)&sa, &salen)) > 0) {
            length -= nread;
            nreaded += nread;
            buffer += nread;
            continue;
        } else if (nread == 0) {
            return -ECONNABORTED;
        }
        break;
    }
    return nread > 0 && !length ? 0 : ((flags & MSG_DONTWAIT) && errno == EAGAIN) ? 0 : -errno;
}

ssize_t csocket::ReadFromSsl([[maybe_unused]] sockaddr_storage& sa, [[maybe_unused]] void* data, [[maybe_unused]] size_t length, [[maybe_unused]] size_t& nread, [[maybe_unused]] uint flags) const {// NOLINT (static)
    return -ESOCKTNOSUPPORT;
}

ssize_t csocket::WriteNoSsl(const void* data, size_t length, size_t& nwrited, uint flags) const {
    ssize_t nwrite {0};
    uint8_t* buffer {(uint8_t*)data};
    nwrited = 0;
    while (length) {
        if ((nwrite = ::send((int)fdSocket, buffer, length, flags | MSG_NOSIGNAL)) > 0) {
            length -= nwrite;
            nwrited += nwrite;
            buffer += nwrite;
            continue;
        } else if (nwrite == -1 and ((flags & MSG_DONTWAIT) && errno == EAGAIN)) {
            // continue;
        }
        break;
    }

    return nwrite > 0 && !length ? 0 : ((flags & MSG_DONTWAIT) && errno == EAGAIN) ? 0 : -errno;
}

ssize_t csocket::WriteSsl(const void* data, size_t length, size_t& nwrited, uint flags) const {
    auto ssl = Ssl();
    if (not ssl) {
        return -EBADF;
    }
    ssize_t nwrite {0};
    uint8_t* buffer {(uint8_t*)data};
    nwrited = 0;
    while (length) {
        if ((nwrite = SSL_write(ssl, buffer, (int)length)) > 0) {
            length -= nwrite;
            nwrited += nwrite;
            buffer += nwrite;
            continue;
        } else if (nwrite == -1 and SSL_get_error(ssl, (int)nwrite) == SSL_ERROR_WANT_WRITE) {
            // continue;
        }
        break;
    }
    switch (int err = SSL_get_error(ssl, (int)nwrite); err) {
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

ssize_t csocket::ReadNoSsl(void* data, size_t length, size_t& nreaded, uint flags) const {
    ssize_t nread {0};
    uint8_t* buffer {(uint8_t*)data};
    nreaded = 0;
    while (length) {
        if ((nread = ::recv((int)fdSocket, buffer, length, flags | MSG_NOSIGNAL)) > 0) {
            length -= nread;
            nreaded += nread;
            buffer += nread;
            continue;
        } else if (nread == 0) {
            return -ECONNABORTED;
        }
        break;
    }

    return nread > 0 && !length ? 0 : ((flags & MSG_DONTWAIT) && errno == EAGAIN) ? 0 : -errno;
}

ssize_t csocket::ReadSsl(void* data, size_t length, size_t& nreaded, uint flags) const {
    auto ssl = Ssl();
    if (not ssl) {
        return -EBADF;
    }
    ssize_t nread {0};
    uint8_t* buffer {(uint8_t*)data};
    nreaded = 0;
    while (length) {
        if ((nread = SSL_read(ssl, buffer, (int)length)) > 0) {
            length -= nread;
            nreaded += nread;
            buffer += nread;
            if (!(flags & MSG_DONTWAIT)) {
                continue;
            }
            return 0;
        }
        break;
    }
    switch (int err = SSL_get_error(ssl, (int)nread); err) {
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

}// namespace inet