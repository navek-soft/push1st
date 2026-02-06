#pragma once
#include "chttpconn.h"

namespace http {

class crequest : protected inet::chttpconnection {
   public:
    crequest(const std::string& url, std::unordered_map<std::string_view, std::string>&& headers, inet::ssl_ctx_t sslCtx) :
        so {-1, {}},
        url {url},
        headers {std::move(headers)},
        sslCtx {std::move(sslCtx)} {}

    ~crequest() override {
        so.SocketClose();
    }

   public:
    http::response_t Call(const std::string& path);

   private:
    inet::csocket so;
    std::string url {};
    std::unordered_map<std::string_view, std::string> headers;
    inet::ssl_ctx_t sslCtx;
};

}// namespace http