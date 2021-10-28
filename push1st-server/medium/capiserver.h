#pragma once
#include "../../lib/http/chttpserver.h"
#include "../../lib/inet/cunixserver.h"
#include "../../lib/http/chttpconn.h"
#include "../core/cconfig.h"
#include "ccredentials.h"
#include "../../lib/http/crouter.h"

class cchannels;
class ccluster;

class capiserver : public inet::chttpconnection
{
	class ctcpapiserver : public inet::chttpserver, public std::enable_shared_from_this<ctcpapiserver> {
	public:
		ctcpapiserver(capiserver& api, config::interface_t config);
		virtual ~ctcpapiserver();
	protected:
		virtual inline void OnHttpRequest(const inet::csocket& fd, const std::string_view& method, const http::uri_t& path, const http::headers_t& headers, const std::string& request, const std::string_view& content) override {
			Api.OnHttpRequest(fd, method, path, headers, request, content);
		}
		virtual void OnHttpError(const inet::csocket& fd, ssize_t err);
		virtual inline std::shared_ptr<inet::ctcpserver> TcpSelf() override { return std::dynamic_pointer_cast<inet::ctcpserver>(shared_from_this()); }
	private:
		capiserver& Api;
	};
	class cunixapiserver : public inet::cunixserver, public inet::chttpconnection, public std::enable_shared_from_this<cunixapiserver> {
	public:
		cunixapiserver(capiserver& api, config::interface_t config, size_t httpMaxHeaderSize = 8192);
		virtual ~cunixapiserver();
	protected:
		void OnHttpRequest(const inet::csocket& fd, const std::string_view& method, const http::uri_t& path, const http::headers_t& headers, const std::string& request, const std::string_view& content) {
			Api.OnHttpRequest(fd, method, path, headers, request, content);
		}
		virtual void OnHttpError(const inet::csocket& fd, ssize_t err);
		virtual inline std::shared_ptr<inet::cunixserver> UnixSelf() override { return std::dynamic_pointer_cast<inet::cunixserver>(shared_from_this()); }
	private:
		virtual ssize_t OnUnixAccept(fd_t fd, const sockaddr_storage& sa, const inet::ssl_t& ssl, const std::weak_ptr<inet::cpoll>& poll) override;
		void OnHttpData(fd_t fd, uint events, const sockaddr_storage& sa, const inet::ssl_t& ssl, const std::weak_ptr<inet::cpoll>& poll);
		capiserver& Api;
		size_t HttpMaxHeaderSize{ 65536 };
	};
public:
	capiserver(const std::shared_ptr<cchannels>& channels, const std::shared_ptr<ccredentials>& credentials, const std::shared_ptr<ccluster>& cluster, config::interface_t config);
	virtual void Listen(const std::shared_ptr<inet::cpoll>& poll) ;
	virtual ~capiserver();
protected:
	friend class ctcpapiserver;
	void OnHttpRequest(const inet::csocket& fd, const std::string_view& method, const http::uri_t& path, const http::headers_t& headers, const std::string& request, const std::string_view& content);
private:
	inline void ApiResponse(const inet::csocket& fd, const std::string_view& code, const std::string& response = {}, bool close = true);
	void ApiOnEvents(const std::vector<std::string_view>&, const inet::csocket&, const std::string_view&, const http::uri_t&, const http::headers_t&, const std::string_view&);
	void ApiOnToken(const std::vector<std::string_view>&, const inet::csocket&, const std::string_view&, const http::uri_t&, const http::headers_t&, const std::string_view&);
	void ApiOnChannels(const std::vector<std::string_view>&, const inet::csocket&, const std::string_view&, const http::uri_t&, const http::headers_t&, const std::string_view&);
	void ApiOnWebHook(const std::vector<std::string_view>&, const inet::csocket&, const std::string_view&, const http::uri_t&, const http::headers_t&, const std::string_view&);
	std::shared_ptr<cchannels> Channels;
	std::shared_ptr<ccredentials> Credentials;
	std::shared_ptr<ccluster> Cluster;
	std::shared_ptr<ctcpapiserver> ApiTcp;
	std::shared_ptr<cunixapiserver> ApiUnix;
	http::crouter ApiRoutes;
	std::string KeepAliveTimeout;
};

