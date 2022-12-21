#include "chttpconn.h"

using namespace inet;

ssize_t chttpconnection::HttpReadRequest(const inet::csocket& fd, std::string_view& method, http::uri_t& path, http::headers_t& headers, std::string& request, std::string_view& content, size_t max_size)
{
	request.resize(max_size);
	ssize_t res{ -1 };
	if (size_t nread, contentLength{ 0 }; (res = fd.SocketRecv(request.data(), request.size(), nread, MSG_DONTWAIT)) == 0) {
		std::string_view data;

		request.resize(nread);

		/* 
		* TODO:	Data can be contains multiple request, create another method with virtual callback handler
		*/

		if (res = http::ParseRequest(std::string_view{ request.data(), nread }, method, path, headers, data, contentLength); res > 0) {
			content = data;
			return 0;
		}
		else if (res == -ENODATA and contentLength) {
			if (http::GetValue(headers, "expect") == "100-continue" and (res = HttpWriteResponse(fd, "100")) == 0) {
				;
			}
			if((request.size() + (contentLength - data.size())) <= max_size) {
				if (res = fd.SocketRecv(request.data() + nread, contentLength - data.length(), nread, 0); res == 0) {
					content = { data.data(), contentLength };
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

ssize_t chttpconnection::HttpWriteRequest(const inet::csocket& fd, const std::string_view& method, const std::string_view& uri, std::unordered_map<std::string_view, std::string>&& headers, const std::string& request) {
	std::string reply;

	reply.reserve(2048 + request.length());
	reply.append(method).append(" ").append(uri).append(" HTTP/1.1").append("\r\n");

	if (!request.empty()) { reply.append("Content-Length: ").append(std::to_string(request.size())).append("\r\n"); }

	HttpServerHeaders(headers);

	for (auto&& [h, v] : headers) { reply.append(h).append(": ").append(v).append("\r\n"); }
	reply.append("\r\n");

	reply.append(request);

	size_t nwrite{ 0 };
	return fd.SocketSend(reply.data(), reply.length(), nwrite, 0);
}


ssize_t chttpconnection::HttpWriteResponse(const inet::csocket& fd, const std::string_view& code, const std::string& response, std::unordered_map<std::string_view, std::string>&& headers) {
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
