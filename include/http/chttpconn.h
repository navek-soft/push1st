#pragma once
#include <http/cresponse.h>
#include <inet/csocket.h>

#include "chttp.h"
#include "chttpconn.h"

namespace inet {
class chttpconnection {
   public:
    chttpconnection() {
        data.reserve(65536);
    }
    virtual ~chttpconnection() = default;

   protected:
    ssize_t HttpReadRequest(const inet::csocket& fd, std::string_view& method, http::uri_t& path, http::headers_t& headers, std::string& request, std::string_view& content, size_t max_size);
    ssize_t HttpReadResponse(const inet::csocket& fd, std::string_view& code, std::string_view& msg, http::headers_t& headers, std::string& response, std::string_view& content, size_t max_size);
    ssize_t HttpReadChunkedResponse(const inet::csocket& fd, std::vector<http::chunk_t>& chunks, size_t max_size);
    ssize_t HttpWriteRequest(const inet::csocket& fd, const std::string_view& method, const std::string_view& uri, std::unordered_map<std::string_view, std::string>&& headers = {}, const std::string& request = "");
    ssize_t HttpWriteResponse(const inet::csocket& fd, const std::string_view& code, const std::string& response = "", std::unordered_map<std::string_view, std::string>&& headers = {});
    virtual inline void HttpServerHeaders([[maybe_unused]] std::unordered_map<std::string_view, std::string>& headers) {
        ;
    }
    virtual inline void OnRequest() {};
    virtual inline void OnResponse(http::response_t&&) {};
    virtual inline void OnResponse(http::chunk_t&&) {};

    inline void Reset() {
        data.clear();
    }

   private:
    std::string data {};
};
}// namespace inet
