#pragma once
#include "chttp.h"
#include "../inet/csocket.h"

namespace http {
	class cconnection
	{
	public:
		cconnection() = default;
		virtual ~cconnection() = default;
	protected:
		ssize_t HttpReadRequest(const inet::csocket& fd, std::string_view& method, http::path_t& path, http::params_t& args, http::headers_t& headers, std::string& request, std::string& content, size_t max_size);
		ssize_t HttpWriteResponse(const inet::csocket& fd, const std::string_view& code, const std::string_view& response = "", std::unordered_map<std::string_view, std::string>&& headers = {});
		virtual inline void HttpServerHeaders(std::unordered_map<std::string_view, std::string>& headers) { ; }
	};
}

