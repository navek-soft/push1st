#pragma once
#include "../http/chttpserver.h"
#include "../core/cconfig.h"
#include "ccredentials.h"

class cchannels;

class capiserver
{
	class ctcpapiserver : public inet::chttpserver, public std::enable_shared_from_this<ctcpapiserver> {
	public:
		ctcpapiserver(capiserver& api, config::interface_t config);
		virtual ~ctcpapiserver();
	protected:
		virtual void OnHttpRequest(const inet::csocket& fd, const std::string_view& method, const http::path_t& path, const http::params_t& args, const http::headers_t& headers, std::string&& request, std::string&& content) override {
			Api.OnHttpRequest(fd, method, path, args, headers, std::move(request), std::move(content));
		}
		virtual inline std::shared_ptr<inet::ctcpserver> TcpSelf() override { return std::dynamic_pointer_cast<inet::ctcpserver>(shared_from_this()); }
	private:
		capiserver& Api;
	};
public:
	capiserver(const std::shared_ptr<cchannels>& channels, const std::shared_ptr<ccredentials>& credentials, config::interface_t config);
	virtual void Listen(const std::shared_ptr<inet::cpoll>& poll) ;
	virtual ~capiserver();
protected:
	friend class ctcpapiserver;
	void OnHttpRequest(const inet::csocket& fd, const std::string_view& method, const http::path_t& path, const http::params_t& args, const http::headers_t& headers, std::string&& request, std::string&& content);
private:
	std::shared_ptr<cchannels> Channels;
	std::shared_ptr<ccredentials> Credentials;
	std::string AppsPath{ "apps" };
	std::shared_ptr<ctcpapiserver> ApiTcp;
};

