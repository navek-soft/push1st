#pragma once
#include <string_view>

#include "chttp.h"
#include "core/util/cjson.h"

namespace http {
struct chunk_t {
    chunk_t(chunk_t&&) noexcept = default;
    chunk_t(std::string&& chunk) : chunk {std::move(chunk)} {}

    json::value_t Json() const {
        json::value_t jsonData {};
        if (not chunk.empty()) {
            json::Unserialize(chunk, jsonData);
        }
        return jsonData;
    }

   private:
    std::string chunk;
};

struct response_t {
    response_t(std::string&& reqData, std::string_view code, std::string_view msg, http::headers_t&& headers, std::string_view body) :
        headers {std::move(headers)},
        code {code},
        msg {msg},
        body {body},
        reqData {std::move(reqData)} {
        ;
    }

    response_t(response_t&&) noexcept = default;

    inline std::string_view Code() const {
        return code;
    }

    inline std::string_view Message() const {
        return msg;
    }

    inline std::string_view Payload() const {
        return body;
    }

    inline const headers_t& Headers() const {
        return headers;
    }

    inline bool KeepAlive() const {
        auto con = Header("connection");
        return con.empty() ? true : (con == "keep-alive");
    }

    inline std::string_view Header(std::string_view header) const {
        if (auto&& it {headers.find(header)}; it != headers.end()) {
            return it->second;
        }
        return {};
    }

    json::value_t Json() const {
        json::value_t jsonData {};
        if (not body.empty()) {
            json::Unserialize(body, jsonData);
        }
        return jsonData;
    }

    inline std::string_view Data() const {
        return {(char*)reqData.c_str(), reqData.size()};
    }

   private:
    http::headers_t headers;
    std::string_view code;
    std::string_view msg;
    std::string_view body;

   private:
    std::string reqData;
};

}// namespace http