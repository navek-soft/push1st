#pragma once
#include <inet/csocket.h>

#include "chttp.h"
#include "chttpconn.h"

namespace inet {
class chttpconnection {
   public:
    chttpconnection() = default;
    virtual ~chttpconnection() = default;

   protected:
    ssize_t HttpReadRequest(const inet::socket_t& fd, std::string_view& method, http::uri_t& path, http::headers_t& headers, std::string& request, std::string_view& content, size_t max_size);
    ssize_t HttpWriteRequest(const inet::socket_t& fd, const std::string_view& method, const std::string_view& uri, std::unordered_map<std::string_view, std::string>&& headers = {}, const std::string& request = "");
    ssize_t HttpWriteResponse(const inet::socket_t& fd, const std::string_view& code, const std::string& response = "", std::unordered_map<std::string_view, std::string>&& headers = {});
    virtual inline void HttpServerHeaders([[maybe_unused]] std::unordered_map<std::string_view, std::string>& headers) {
        ;
    }
};
}// namespace inet
