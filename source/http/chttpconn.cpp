#include <http/chttpconn.h>
#include <unistd.h>

#include <cassert>
#include <cerrno>
#include <cstddef>
#include <string>
#include <string_view>

#include "http/chttp.h"
#include "log/clog.h"

using namespace inet;

ssize_t chttpconnection::HttpReadRequest(const inet::csocket& fd, std::string_view& method, http::uri_t& path, http::headers_t& headers, std::string& request, std::string_view& content, size_t max_size) {
    request.resize(max_size);
    ssize_t res {-1};
    if (size_t nread, contentLength {0}; (res = fd.SocketRecv(request.data(), request.size(), nread, MSG_DONTWAIT)) == 0) {
        std::string_view data;

        request.resize(nread);

        /*
         * TODO:	Data can be contains multiple request, create another method with virtual callback handler
         */

        if (res = http::ParseRequest(std::string_view {request.data(), nread}, method, path, headers, data, contentLength); res > 0) {
            content = data;
            return 0;
        } else if (res == -ENODATA and contentLength) {
            if (http::GetValue(headers, "expect") == "100-continue" and (res = HttpWriteResponse(fd, "100")) == 0) {// NOLINT (value is never actually read from 'res')
                ;
            }
            if ((request.size() + (contentLength - data.size())) <= max_size) {
                if (res = fd.SocketRecv(request.data() + nread, contentLength - data.length(), nread, 0); res == 0) {
                    content = {data.data(), contentLength};
                    return 0;
                }
            } else {
                return -EMSGSIZE;
            }
        }
    }
    return res;
}

ssize_t chttpconnection::HttpReadChunkedResponse(const inet::csocket& fd, std::vector<http::chunk_t>& chunks, size_t max_size) {// NOLINT
    ssize_t res {-1};

    std::string chunkStr = std::move(data);
    data.clear();
    std::string response;
    response.resize(max_size);

    if (size_t nread; (res = fd.SocketRecv(response.data(), response.size(), nread, MSG_DONTWAIT)) == 0) {
        auto&& body = std::string_view {response.data(), nread};

        while (not body.empty()) {
            auto&& chunkLength = http::FindFirstOf(body, "\r\n");
            if (size_t length {0}; not chunkLength.empty() and ((length = http::FromHex(chunkLength)) != 0)) {
                if (body.size() < length + 2) {// chunk len + 2 delimiters at the end of the chunk
                    data.append(chunkStr).append(body);
                    data.shrink_to_fit();
                    return -ENODATA;
                }

                chunkStr.append(body.substr(0, length));
                body.remove_prefix(length + 2);// remove "\r\n"
            } else if (not chunkLength.empty()) {
                chunkStr.append(chunkLength);
            } else {
                chunkStr.append(body);
                body.remove_prefix(body.size());
            }

            ssize_t n {0};
            while ((n = http::ExtractJson(chunkStr)) > 0) {
                chunks.emplace_back(chunkStr.substr(0, n + 1));
                chunkStr.erase(0, n + 1);
            }
        }

        while (not chunkStr.empty()) {
            if (chunkStr[0] == '\n' or chunkStr[0] == '\r') {
                chunkStr.erase(0, 1);
                continue;
            }
            break;
        }

        data = std::move(chunkStr);
        return 0;
    } else {
        return res;
    }
}

ssize_t chttpconnection::HttpReadResponse(const inet::csocket& fd, std::string_view& code, std::string_view& msg, http::headers_t& headers, std::string& response, std::string_view& content, size_t max_size) {// NOLINT
    response.resize(max_size);
    ssize_t res {-1};

    if (size_t nread, contentLength {0}; (res = fd.SocketRecv(response.data(), response.size(), nread, MSG_DONTWAIT)) == 0) {
        std::string_view data;

        if (res = http::ParseResponse(std::string_view {response.data(), nread}, code, msg, headers, contentLength, data); res > 0) {
            content = data;
            return 0;
        } else if (res == -ENODATA) {
            if (contentLength and (response.size() + (contentLength - data.size())) <= max_size) {
                if (res = fd.SocketRecv(response.data() + nread, contentLength - data.length(), nread, MSG_DONTWAIT); res == 0) {
                    content = {data.data(), contentLength};
                    return 0;
                }
            } else if (not contentLength) {// chunked response
                return 0;
            }
        }
    }

    return res;
}

ssize_t chttpconnection::HttpWriteRequest(const inet::csocket& fd, const std::string_view& method, const std::string_view& uri, std::unordered_map<std::string_view, std::string>&& headers, const std::string& request) {
    std::string reply;

    reply.reserve(2048 + request.length());
    reply.append(method).append(" ").append(uri).append(" HTTP/1.1").append("\r\n");

    if (!request.empty()) {
        reply.append("Content-Length: ").append(std::to_string(request.size())).append("\r\n");
    }

    HttpServerHeaders(headers);

    for (auto&& [h, v] : headers) {
        reply.append(h).append(": ").append(v).append("\r\n");
    }
    reply.append("\r\n");

    reply.append(request);

    size_t nwrite {0};
    return fd.SocketSend(reply.data(), reply.length(), nwrite, 0);
}

ssize_t chttpconnection::HttpWriteResponse(const inet::csocket& fd, const std::string_view& code, const std::string& response, std::unordered_map<std::string_view, std::string>&& headers) {
    std::string reply;

    reply.reserve(2048);
    reply.append("HTTP/1.1 ").append(code).append(" ").append(http::GetMessage(code)).append("\r\n");

    if (!response.empty()) {
        reply.append("Content-Length: ").append(std::to_string(response.size())).append("\r\n");
    }

    HttpServerHeaders(headers);

    for (auto&& [h, v] : headers) {
        reply.append(h).append(": ").append(v).append("\r\n");
    }
    reply.append("\r\n");

    size_t nwrite {0};
    ssize_t res {-1};
    if (res = fd.SocketSend(reply.data(), reply.length(), nwrite, 0); res == 0) {
        if (!response.empty()) {
            res = fd.SocketSend(response.data(), response.length(), nwrite, 0);
        }
    }
    return res;
}
