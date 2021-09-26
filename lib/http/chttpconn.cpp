#include "chttpconn.h"

using namespace inet;

ssize_t chttpconnection::HttpReadRequest(const inet::csocket& fd, std::string_view& method, http::path_t& path, http::params_t& args, http::headers_t& headers, std::string& request, std::string& content, size_t max_size)
{
	request.resize(max_size);
	ssize_t res{ -1 };
	if (size_t nread, contentLength{ 0 }; (res = fd.SocketRecv(request.data(), request.size(), nread, MSG_DONTWAIT)) == 0) {
		std::string_view data;

		request.resize(nread);

		if (res = http::ParseRequest(std::string_view{ request.data(), request.size() }, method, path, args, headers, data, contentLength); res > 0) {
			content = data;
			return 0;
		}
		else if (res == -ENODATA and contentLength) {
			if (http::GetValue(headers, "expect") == "100-continue" and (res = HttpWriteResponse(fd, "100")) == 0) {
				;
			}
			else {
				return res;
			}

			if (((nread + contentLength) - data.length()) < max_size) {
				content.resize(contentLength);
				std::memcpy(content.data(), data.data(), data.length());
				if (res = fd.SocketRecv(content.data() + data.length(), content.size() - data.length(), nread, 0); res == 0) {
					return 0;
				}
			}
			else {
				return -EMSGSIZE;
			}
		}
	}
	return res;
}

ssize_t chttpconnection::HttpWriteResponse(const inet::csocket& fd, const std::string_view& code, const std::string_view& response, std::unordered_map<std::string_view, std::string>&& headers) {
	std::string reply;

	reply.reserve(2048);
	reply.append("HTTP/1.1 ").append(code).append(" ").append(http::GetMessage(code)).append("\r\n");

	if (!response.empty()) { reply.append("Content-Length: ").append(std::to_string(response.size())).append("\r\n"); }

	HttpServerHeaders(headers);

	for (auto&& [h, v] : headers) { reply.append(h).append(": ").append(v).append("\r\n"); }
	reply.append("\r\n");

	size_t nwrite{ 0 };
	ssize_t res{ -1 };
	if (res = fd.SocketSend(reply.data(), reply.length(), nwrite, 0); res == 0) {
		if (!response.empty()) {
			res = fd.SocketSend(response.data(), response.length(), nwrite,0 );
		}
	}
	return res;
}
