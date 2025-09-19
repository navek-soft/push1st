#pragma once
#include "cepoll.h"
#include "cinet.h"

namespace inet {
class csocket {
   public:
    csocket(fd_t so, const sockaddr_storage& sa, const inet::ssl_t& ssl, const std::weak_ptr<inet::cpoll>& poll) :
        fdSocket {so},
        fdSsl {ssl},
        fdPoll {poll} {
        if (ssl) {
            write_fn = &csocket::WriteSsl;
            read_fn = &csocket::ReadSsl;
        }
        memcpy((void*)&sa, &fdSa, sizeof(sockaddr_storage));
    }
    csocket(fd_t so, const inet::ssl_t& ssl) : fdSocket {so}, fdSsl {ssl} {
        if (ssl) {
            write_fn = &csocket::WriteSsl;
            read_fn = &csocket::ReadSsl;
        }
    }
    csocket(const csocket&) = delete;
    csocket(const csocket&& so) noexcept :
        fdSocket {so.fdSocket},
        fdSsl {std::move(so.fdSsl)},
        fdPoll {std::move(so.fdPoll)}// NOLINT (std::move have no effect, but i cant delete it)
    {
        so.fdSocket = -1;
        so.fdSsl.reset();
        memcpy((void*)&so.fdSa, &fdSa, sizeof(sockaddr_storage));
        if (fdSsl) {
            write_fn = &csocket::WriteSsl;
            read_fn = &csocket::ReadSsl;
        }
    }

    inline csocket& operator=(csocket&& so) noexcept;
    inline csocket& operator=(fd_t so);
    virtual ~csocket() {
        ;
    }
    virtual inline void OnSocketConnect() {
        ;
    }
    virtual inline void OnSocketRecv() {
        ;
    }
    virtual inline void OnSocketSend() {
        ;
    }
    virtual inline void OnSocketError([[maybe_unused]] ssize_t err) {
        ;
    }
    inline operator bool() const {
        return fdSocket > 0;
    }

   public:
    inline void SocketUpdateEvents(uint events);
    inline ssize_t SocketSend(const void* data, size_t length, size_t& nwrite, uint flags) const {
        return (this->*write_fn)(data, length, nwrite, flags);
    }
    inline ssize_t SocketRecv(void* data, size_t length, size_t& nread, uint flags) const {
        return (this->*read_fn)(data, length, nread, flags);
    }
    inline ssize_t SocketSend(const sockaddr_storage& sa, const void* data, size_t length, size_t& nwrite, uint flags) const {
        return (this->*write_to_fn)(sa, data, length, nwrite, flags);
    }
    inline ssize_t SocketRecv(sockaddr_storage& sa, void* data, size_t length, size_t& nread, uint flags) const {
        return (this->*read_from_fn)(sa, data, length, nread, flags);
    }
    inline int Fd() const {
        return (int)fdSocket;
    }
    inline auto Poll() const {
        return fdPoll.lock();
    }
    inline const sockaddr_storage& Address() const {
        return fdSa;
    }
    int SocketClose() const;
    inline std::string GetAddress() const {
        return inet::GetIp(fdSa);
    }
    inline uint32_t GetIp4() const {
        return inet::GetIp4(fdSa);
    }
    inline uint8_t* GetIp6() const {
        return inet::GetIp6(fdSa);
    }
    inline int GetAF() const {
        return inet::GetAF(fdSa);
    }
    inline uint16_t GetPort() const {
        return inet::GetPort(fdSa);
    }
    inline ssize_t GetErrorNo() const {
        return inet::GetErrorNo(fdSocket);
    }
    inline ssize_t SetKeepAlive(bool enable, int count, int idle_sec, int intrvl_sec) const {
        return inet::SetKeepAlive(fdSocket, enable, count, idle_sec, intrvl_sec);
    }
    inline ssize_t SetNonBlock(bool nonblock) const {
        return inet::SetNonBlock(fdSocket, nonblock);
    }
    inline ssize_t SetRecvTimeout(std::time_t milliseconds) const {
        return inet::SetRecvTimeout(fdSocket, (size_t)milliseconds);
    }
    inline ssize_t SetSendTimeout(std::time_t milliseconds) const {
        return inet::SetSendTimeout(fdSocket, (size_t)milliseconds);
    }

   private:
    mutable fd_t fdSocket;
    mutable std::shared_ptr<struct ssl_st> fdSsl;
    sockaddr_storage fdSa;
    std::weak_ptr<inet::cpoll> fdPoll;

    inline struct ssl_st* Ssl() const {
        return fdSsl.get();
    }

    ssize_t WriteNoSsl(const void* data, size_t length, size_t& nwrite, uint flags) const;
    ssize_t WriteSsl(const void* data, size_t length, size_t& nwrite, uint flags) const;
    ssize_t ReadNoSsl(void* data, size_t length, size_t& nread, uint flags) const;
    ssize_t ReadSsl(void* data, size_t length, size_t& nread, uint flags) const;
    ssize_t WriteToNoSsl(const sockaddr_storage& sa, const void* data, size_t length, size_t& nwrite, uint flags) const;
    ssize_t WriteToSsl(const sockaddr_storage& sa, const void* data, size_t length, size_t& nwrite, uint flags) const;
    ssize_t ReadFromNoSsl(sockaddr_storage& sa, void* data, size_t length, size_t& nread, uint flags) const;
    ssize_t ReadFromSsl(sockaddr_storage& sa, void* data, size_t length, size_t& nread, uint flags) const;

    ssize_t (csocket::*write_fn)(const void* data, size_t length, size_t& nwrite, uint flags) const {&csocket::WriteNoSsl};
    ssize_t (csocket::*read_fn)(void* data, size_t length, size_t& nread, uint flags) const {&csocket::ReadNoSsl};
    ssize_t (csocket::*write_to_fn)(const sockaddr_storage& sa, const void* data, size_t length, size_t& nwrite, uint flags) const {&csocket::WriteToNoSsl};
    ssize_t (csocket::*read_from_fn)(sockaddr_storage& sa, void* data, size_t length, size_t& nread, uint flags) const {&csocket::ReadFromNoSsl};
};

inline csocket& csocket::operator=(csocket&& so) noexcept {
    fdSocket = so.fdSocket;
    fdSsl = so.fdSsl;
    fdPoll = so.fdPoll;
    so.fdSocket = -1;
    so.fdSsl.reset();
    memcpy(&so.fdSa, &fdSa, sizeof(sockaddr_storage));
    if (fdSsl) {
        write_fn = &csocket::WriteSsl;
        read_fn = &csocket::ReadSsl;
    }
    return *this;
}

inline csocket& csocket::operator=(fd_t so) {
    fdSocket = so;
    fdSsl.reset();
    fdPoll.reset();
    memset(&fdSa, 0, sizeof(sockaddr_storage));// fix
    if (fdSsl) {
        write_fn = &csocket::WriteSsl;
        read_fn = &csocket::ReadSsl;
    }
    return *this;
}

inline void csocket::SocketUpdateEvents(uint events) {
    if (auto&& poll {fdPoll.lock()}; poll and fdSocket > 0) {
        if (auto res = poll->PollUpdate(fdSocket, events); res == 0) {
            return;
        } else {
            OnSocketError(res);
        }
    } else {
        OnSocketError(-EBADFD);
    }
}

using socket_t = std::shared_ptr<csocket>;
}// namespace inet
