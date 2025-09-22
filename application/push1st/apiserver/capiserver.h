#pragma once
#include <memory>

#include "core/config/cconfig.h"
#include "core/util/cfactory.h"
#include "http/chttpconn.h"
#include "http/chttpserver.h"
#include "http/crouter.h"
#include "inet/cunixserver.h"
#include "log/clog.h"

class cchannels;
class ccluster;
class csmppservice;
class ccredentials;

class capiserver : public inet::chttpconnection, public csharedfactory<capiserver>, public std::enable_shared_from_this<capiserver> {
    friend csharedfactory<capiserver>;

    log_as(apiserver);
    class ctcpapiserver : public inet::chttpserver, public std::enable_shared_from_this<ctcpapiserver> {
        log_as(tcpapiserver);

       public:
        ctcpapiserver(capiserver& api, config::interface_t config);
        ~ctcpapiserver() override;

       protected:
        inline void OnHttpRequest(const inet::socket_t& fd, const std::string_view& method, const http::uri_t& path, const http::headers_t& headers, const std::string& request, const std::string_view& content) override {
            Api.OnHttpRequest(fd, method, path, headers, request, content);
        }
        void OnHttpError(const inet::socket_t& fd, ssize_t err) override;
        inline std::shared_ptr<inet::ctcpserver> TcpSelf() override {
            return std::dynamic_pointer_cast<inet::ctcpserver>(shared_from_this());
        }

       private:
        capiserver& Api;
    };
    class cunixapiserver : public inet::cunixserver, public inet::chttpconnection, public std::enable_shared_from_this<cunixapiserver> {
        log_as(unixapiserver);

       public:
        cunixapiserver(capiserver& api, config::interface_t config, size_t httpMaxHeaderSize = 8192);
        ~cunixapiserver() override;

       public:
        void Join() {
            inet::cunixserver::UnixJoin();
        }

       protected:
        void OnHttpRequest(const inet::socket_t& fd, const std::string_view& method, const http::uri_t& path, const http::headers_t& headers, const std::string& request, const std::string_view& content) {
            Api.OnHttpRequest(fd, method, path, headers, request, content);
        }
        virtual void OnHttpError(const inet::socket_t& fd, ssize_t err);
        inline std::shared_ptr<inet::cunixserver> UnixSelf() override {
            return std::dynamic_pointer_cast<inet::cunixserver>(shared_from_this());
        }

       private:
        ssize_t OnUnixAccept(fd_t fd, const sockaddr_storage& sa, const inet::ssl_t& ssl, const std::weak_ptr<inet::cpoll>& poll) override;
        void OnHttpData(fd_t fd, uint events, const sockaddr_storage& sa, const inet::ssl_t& ssl, const std::weak_ptr<inet::cpoll>& poll);
        capiserver& Api;
        size_t HttpMaxHeaderSize {65536};
    };

   protected:
    capiserver(const std::shared_ptr<cchannels>& channels, const std::shared_ptr<ccredentials>& credentials, const std::shared_ptr<ccluster>& cluster, config::interface_t config);

   public:
    void Join();

    ~capiserver() override;

   protected:
    friend class ctcpapiserver;
    void OnHttpRequest(const inet::socket_t& fd, const std::string_view& method, const http::uri_t& path, const http::headers_t& headers, const std::string& request, const std::string_view& content);

   private:
    inline void ApiResponse(const inet::socket_t& fd, const std::string_view& code, const std::string& response = {}, bool close = true);
    void ApiOnEvents(const std::vector<std::string_view>&, const inet::socket_t&, const std::string_view&, const http::uri_t&, const http::headers_t&, const std::string_view&);
    void ApiOnToken(const std::vector<std::string_view>&, const inet::socket_t&, const std::string_view&, const http::uri_t&, const http::headers_t&, const std::string_view&);
    void ApiOnChannels(const std::vector<std::string_view>&, const inet::socket_t&, const std::string_view&, const http::uri_t&, const http::headers_t&, const std::string_view&);
    void ApiOnWebHook(const std::vector<std::string_view>&, const inet::socket_t&, const std::string_view&, const http::uri_t&, const http::headers_t&, const std::string_view&);
    void ApiOnModuleSmpp(const std::vector<std::string_view>&, const inet::socket_t&, const std::string_view&, const http::uri_t&, const http::headers_t&, const std::string_view&);

   private:
    std::shared_ptr<cchannels> Channels;
    std::shared_ptr<ccredentials> Credentials;
    std::shared_ptr<ccluster> Cluster;
    std::shared_ptr<ctcpapiserver> ApiTcp;
    std::shared_ptr<cunixapiserver> ApiUnix;
    std::shared_ptr<csmppservice> ApiSmpp;
    http::crouter ApiRoutes;
    std::string KeepAliveTimeout;
};
