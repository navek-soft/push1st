#include <http/chttprequest.h>

#include <cerrno>

#include "http/cresponse.h"
#include "log/clog.h"

namespace http {

http::response_t crequest::Call(const std::string& path) {
    Reset();

    inet::ssl_t fdSsl;
    fd_t fd {-1};
    sockaddr_storage sa {};

    if (auto&& res = inet::GetSockAddr(*const_cast<sockaddr_storage*>(&sa), url, "0", AF_INET); res < 0) {
        throw std::runtime_error(fmt::format("can't get http address on {} error {}:{}", url, -res, std::strerror(-(int)res)));
    }

    if (ssize_t res; sslCtx) {
        if (res = inet::SslConnect(fd, sa, false, 1500, sslCtx, fdSsl); res != 0) {
            throw std::runtime_error(fmt::format("Ssl connect to k8s {}  ... error ( {}:{} )", inet::GetAddress(sa), std::strerror(-(int)res), inet::SslGetErrorString()));
        }
    } else if (res = inet::TcpConnect(fd, sa, false, 10000); res != 0) {
        throw std::runtime_error(fmt::format("Connect to k8s {}  ... error ( {} )", inet::GetAddress(sa), std::strerror(-(int)res)));
    }

    so = inet::csocket {fd, sa, sslCtx ? fdSsl : inet::ssl_t {}, {}};
    so.SetKeepAlive(true, 3, 10, 3);
    so.SetRecvTimeout(3000);
    so.SetSendTimeout(1000);

    if (auto&& res = HttpWriteRequest(so, "GET", path, std::move(headers), ""); res < 0) {
        throw std::runtime_error(fmt::format("Send '{}' to k8s {}  ... error ( {} )", path, inet::GetAddress(sa), std::strerror(-(int)res)));
    }

    std::string response;
    std::string_view code;
    std::string_view msg;
    http::headers_t headers;
    std::string_view content;
    constexpr size_t max_size = 65536;

    if (auto&& res = HttpReadResponse(so, code, msg, headers, response, content, max_size); res >= 0) {
        if (headers.contains("transfer-encoding") and headers.at("transfer-encoding") == "chunked") {
            std::vector<chunk_t> chunks {};
            while (true) {
                if (res = HttpReadChunkedResponse(so, chunks, max_size); res < 0 and res != -ENODATA) {
                    break;
                }
                if (not chunks.empty()) {
                    return http::response_t {std::move(response), code, msg, std::move(headers), chunks.begin()->Raw()};
                }
            }
            throw std::runtime_error(fmt::format("K8s chunked reply error: {} - {}", std::strerror(-(int)res), response));
        }
        return http::response_t {std::move(response), code, msg, std::move(headers), std::string {content}};
    } else {
        throw std::runtime_error(fmt::format("K8s reply error: {} - {}", std::strerror(-(int)res), response));
    }
}

}// namespace http