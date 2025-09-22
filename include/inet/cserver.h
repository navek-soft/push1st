#pragma once
#include <unistd.h>

#include "cinet.h"

namespace inet {
class cpoll;
class cserver {
   public:
    cserver(const std::string& name) : SrvName {name} {
        ;
    }
    virtual ~cserver() {
        ;
    }

   protected:
    virtual std::shared_ptr<cserver> GetInstance() = 0;
    virtual ssize_t Accept(fd_t, std::weak_ptr<cpoll> poll) = 0;
    virtual ssize_t AcceptSSL(fd_t, std::weak_ptr<cpoll> poll) = 0;
    inline const char* NameOf() {
        return SrvName.c_str();
    }

   protected:
    std::string SrvName;
};
}// namespace inet

using server_t = inet::cserver;