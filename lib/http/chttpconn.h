#pragma once
#include "chttp.h"
#include "../inet/csocket.h"

namespace inet {
	class chttpconnection
	{
	public:
		chttpconnection() = default;
		virtual ~chttpconnection() = default;
	protected:
		ssize_t HttpReadRequest(const inet::csocket& fd, std::string_view& method, http::uri_t& path, http::headers_t& headers, std::string& request, std::string& content, size_t max_size);
		ssize_t HttpWriteRequest(const inet::csocket& fd, const std::string_view& method, const std::string_view& uri, std::unordered_map<std::string_view, std::string>&& headers = {}, const std::string_view& request = "");
		ssize_t HttpWriteResponse(const inet::csocket& fd, const std::string_view& code, const std::string_view& response = "", std::unordered_map<std::string_view, std::string>&& headers = {});
		virtual inline void HttpServerHeaders(std::unordered_map<std::string_view, std::string>& headers) { ; }
	};
}

