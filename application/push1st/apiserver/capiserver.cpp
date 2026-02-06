#include "capiserver.h"

#include <cstring>
#include <memory>

#include "channels/cchannel.h"
#include "channels/cchannels.h"
#include "core/cluster/ccluster.h"
#include "core/config/cconfig.h"
#include "core/credentials/ccredentials.h"
#include "core/util/casyncqueue.h"
#include "log/clog.h"

#include "../smppservice/csmppservice.h"

static core::casyncqueue ApiProcessing {4, "apimgm"};

static inline message_t DupChannelMessage(const json::value_t& msg, const std::string& channel) {
    message_t&& out {std::make_shared<json::value_t>(json::object_t {})};
    (*out)["channel"] = channel;
    (*out)["event"] = msg["name"];
    (*out)["data"] = msg["data"];
    (*out)["#msg-id"] = msg["#msg-id"];
    (*out)["#msg-from"] = msg["#msg-from"];
    (*out)["#msg-arrival"] = msg["#msg-arrival"];
    (*out)["#msg-expired"] = msg["#msg-expired"];
    (*out)["#msg-delivery"] = msg["#msg-delivery"];
    if (msg.contains("socket_id")) {
        (*out)["socket_id"] = msg["socket_id"];
    }
    if (msg.contains("to_socket_id")) {
        (*out)["to_socket_id"] = msg["to_socket_id"];
    }
    return out;
}

void capiserver::ApiOnEvents(const std::vector<std::string_view>& vals, const inet::socket_t& fd, const std::string_view& method, [[maybe_unused]] const http::uri_t& uri, const http::headers_t& headers, const std::string_view& content) {
    if (method == "POST") {
        auto&& tmStart {std::chrono::system_clock::now().time_since_epoch().count()};
        if (auto&& app {Credentials->GetAppById(std::string {vals[0]})}; app) {
            if (message_t message; !content.empty() and (message = msg::Unserialize(content, "api"))) {
                auto&& msg = *message;
                if (msg.contains("channel") and msg["channel"].is_string()) {
                    msg["channels"] = json::array_t {msg["channel"].get<std::string>()};
                }
                if (msg.contains("name") and msg.contains("channels") and msg["channels"].is_array()) {
                    json::object_t reply;
                    reply["msg-id"] = msg["#msg-id"];
                    reply["channels"] = json::object_t {};
                    std::string chName;
                    for (auto&& chIt : msg["channels"]) {
                        chName = chIt.get<std::string>();
                        if (auto&& ch {Channels->Get(app, chName)}; ch) {
                            reply["channels"][chName] = ch->Push(DupChannelMessage(msg, chName));
                        } else {
                            reply["channels"][chName] = (size_t)0;
                        }
                        Cluster->Push(ChannelType(chName), app, chName, *DupChannelMessage(msg, chName).get());
                    }
                    reply["time"] = std::chrono::system_clock::now().time_since_epoch().count() - tmStart;
                    ApiResponse(fd, "200", json::Serialize(reply), http::IsConnectionKeepAlive(headers));
                } else {
                    ApiResponse(fd, "400");
                }
            } else {
                ApiResponse(fd, "400");
            }
        } else {
            ApiResponse(fd, "403");
        }
    } else {
        ApiResponse(fd, "400");
    }
}

void capiserver::ApiOnChannels(const std::vector<std::string_view>& vals, const inet::socket_t& fd, const std::string_view& method, [[maybe_unused]] const http::uri_t& uri, const http::headers_t& headers, [[maybe_unused]] const std::string_view& content) {
    if (method == "GET") {
        if (auto&& app {Credentials->GetAppById(std::string {vals[0]})}; app) {
            if (vals[1].empty()) {
                json::array_t chList;
                for (auto&& [chName, chObj] : Channels->List()) {
                    /* All channels list, filter by app-id */
                    if (std::strncmp(app->Id.data(), chName.data(), app->Id.length()) == 0) {
                        chList.emplace_back(chObj->ApiStats());
                    }
                }
                ApiResponse(fd, "200", json::Serialize(chList), http::IsConnectionKeepAlive(headers));
            } else if (auto&& ch {Channels->Get(app, std::string {vals[1]})}; ch) {
                ApiResponse(fd, "200", json::Serialize(ch->ApiOverview()), http::IsConnectionKeepAlive(headers));
            } else {
                ApiResponse(fd, "404");
            }
        } else {
            ApiResponse(fd, "403");
        }
    } else {
        ApiResponse(fd, "400");
    }
}

void capiserver::ApiOnToken(const std::vector<std::string_view>& vals, const inet::socket_t& fd, const std::string_view& method, [[maybe_unused]] const http::uri_t& uri, [[maybe_unused]] const http::headers_t& headers, const std::string_view& content) {
    if (method == "POST") {
        if (auto&& app {Credentials->GetAppById(std::string {vals[0]})}; app) {
            if (vals[1] == "session") {
                if (json::value_t request; cjson::Unserialize(content, request) and request.is_object() and request.contains("session")
                                           and request.contains("channel")) {
                    auto&& token {app->SessionToken(request["session"].get<std::string>(),
                                                    request["channel"].get<std::string>(),
                                                    !request.contains("data") ? std::string {} : request["data"].get<std::string>())};
                    ApiResponse(fd, "200", token);
                } else {
                    ApiResponse(fd, "400");
                }
            } else if (vals[1] == "access") {
                if (json::value_t request; cjson::Unserialize(content, request) and request.is_object() and request.contains("origin")
                                           and request.contains("channel") and request.contains("ttl")) {
                    auto&& token {app->AccessToken(request["origin"].get<std::string>(),
                                                   request["channel"].get<std::string>(),
                                                   request["ttl"].is_number() ? request["ttl"].get<size_t>() : 0)};
                    ApiResponse(fd, "200", token);
                } else {
                    ApiResponse(fd, "400");
                }
            }
        } else {
            ApiResponse(fd, "403");
        }
    } else {
        ApiResponse(fd, "400");
    }
}

void capiserver::ApiOnModuleSmpp(const std::vector<std::string_view>& vals, const inet::socket_t& fd, const std::string_view& method, [[maybe_unused]] const http::uri_t& uri, [[maybe_unused]] const http::headers_t& headers, const std::string_view& content) {
    if (method == "POST") {
        if (auto&& app {Credentials->GetAppById(std::string {vals[0]})}; app) {
            if (json::value_t request; cjson::Unserialize(content, request) and request.is_object()) {
                ApiProcessing.Enqueue([this, weak = weak_from_this(), fd, request]() {
                    if (auto&& self = weak.lock(); self) {
                        auto&& [code, response] = ApiSmpp->Send(request);
                        ApiResponse(fd, code, cjson::Serialize(std::move(response)), true);
                    }
                });
            } else {
                ApiResponse(fd, "400", {}, true);
            }
        } else {
            ApiResponse(fd, "403", {}, true);
        }
    } else {
        ApiResponse(fd, "400", {}, true);
    }
}

void capiserver::ApiOnWebHook([[maybe_unused]] const std::vector<std::string_view>& vals, const inet::socket_t& fd, [[maybe_unused]] const std::string_view& method, [[maybe_unused]] const http::uri_t& uri, const http::headers_t& headers, [[maybe_unused]] const std::string_view& content) {
    ApiResponse(fd, "200", {}, http::IsConnectionKeepAlive(headers));
}

void capiserver::ApiResponse(const inet::socket_t& fd, const std::string_view& code, const std::string& response, bool close) {
    if (close or KeepAliveTimeout.empty()) {
        HttpWriteResponse(*fd, code, response, {{"Connection", "Close"}, {"Content-Type", "application/json; charset=utf-8"}});
        fd->SocketClose();
    } else {
        HttpWriteResponse(*fd, code, response, {{"Connection", "Keep-Alive"}, {"Content-Type", "application/json; charset=utf-8"}, {"Keep-Alive", "timeout=" + KeepAliveTimeout}});
    }
}

/// MAIN
void capiserver::OnHttpRequest(const inet::socket_t& fd, const std::string_view& method, const http::uri_t& path, const http::headers_t& headers, [[maybe_unused]] const std::string& request, const std::string_view& content) {
    PSHT_DEBUG("fd {} method {} uri {}", fd->Fd(), std::string {method}.c_str(), std::string {path.uriFull}.c_str());
    try {
        if (!ApiRoutes.Call(fd, method, path, headers, content)) {
            HttpWriteResponse(*fd, "404");
            fd->SocketClose();
        }
    } catch (std::exception& ex) {
        PSHT_ERROR("fd {} method {} uri {}: {}", fd->Fd(), std::string {method}.c_str(), std::string {path.uriFull}.c_str(), ex.what());
        HttpWriteResponse(*fd, "400");
        fd->SocketClose();
    }
}

void capiserver::Join() {
    if (ApiTcp) {
        ApiTcp->Join();
    }
    if (ApiUnix) {
        ApiUnix->Join();
    }
}

capiserver::capiserver(const std::shared_ptr<cchannels>& channels, const std::shared_ptr<ccredentials>& credentials, const std::shared_ptr<ccluster>& cluster, config::interface_t config) :
    Channels {channels},
    Credentials {credentials},
    Cluster {cluster},
    KeepAliveTimeout {std::to_string(config.KeepAlive)} {
    if (!config.Tcp.Empty()) {
        ApiTcp = std::make_shared<ctcpapiserver>(*this, config);
    }
    if (!config.Unix.Empty()) {
        ApiUnix = std::make_shared<cunixapiserver>(*this, config);
    }

    /* Push event endpoint */
    ApiRoutes.Add("/" + config.Path + "/?/events/*", [this](auto&& PH1, auto&& PH2, auto&& PH3, auto&& PH4, auto&& PH5, auto&& PH6) {
        ApiOnEvents(std::forward<decltype(PH1)>(PH1), std::forward<decltype(PH2)>(PH2), std::forward<decltype(PH3)>(PH3), std::forward<decltype(PH4)>(PH4), std::forward<decltype(PH5)>(PH5), std::forward<decltype(PH6)>(PH6));
    });
    /* Generate private\presense session token */
    ApiRoutes.Add("/" + config.Path + "/?/token/?/", [this](auto&& PH1, auto&& PH2, auto&& PH3, auto&& PH4, auto&& PH5, auto&& PH6) {
        ApiOnToken(std::forward<decltype(PH1)>(PH1), std::forward<decltype(PH2)>(PH2), std::forward<decltype(PH3)>(PH3), std::forward<decltype(PH4)>(PH4), std::forward<decltype(PH5)>(PH5), std::forward<decltype(PH6)>(PH6));
    });

    ApiRoutes.Add("/" + config.Path + "/?/channels/?/", [this](auto&& PH1, auto&& PH2, auto&& PH3, auto&& PH4, auto&& PH5, auto&& PH6) {
        ApiOnChannels(std::forward<decltype(PH1)>(PH1), std::forward<decltype(PH2)>(PH2), std::forward<decltype(PH3)>(PH3), std::forward<decltype(PH4)>(PH4), std::forward<decltype(PH5)>(PH5), std::forward<decltype(PH6)>(PH6));
    });

    if (config.Smpp.Enable and !config.Smpp.Path.empty()) {
        ApiSmpp = std::make_shared<csmppservice>(config.Smpp.Hook);
        ApiRoutes.Add("/" + config.Smpp.Path + "/*", [this](auto&& PH1, auto&& PH2, auto&& PH3, auto&& PH4, auto&& PH5, auto&& PH6) {
            ApiOnModuleSmpp(std::forward<decltype(PH1)>(PH1), std::forward<decltype(PH2)>(PH2), std::forward<decltype(PH3)>(PH3), std::forward<decltype(PH4)>(PH4), std::forward<decltype(PH5)>(PH5), std::forward<decltype(PH6)>(PH6));
        });

        ApiSmpp->Listen();
    }

#ifdef DEBUG1
    ApiRoutes.add("/webhook/*", std::bind(&capiserver::ApiOnWebHook, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6));
#endif// DEBUG
}

capiserver::~capiserver() {}

void capiserver::ctcpapiserver::OnHttpError(const inet::socket_t& fd, ssize_t err) {
    PSHT_INFO("Tcp server request error ( {} )", std::strerror(-(int)err));
    fd->SocketClose();
}

capiserver::ctcpapiserver::ctcpapiserver(capiserver& api, config::interface_t config) :
    inet::chttpserver {"tapi:srv", config.Threads, config.Accept, std::string {config.Tcp.HostPort()}, config.Ssl.Context(), MaxHttpHeaderSize},
    Api {api} {
    PSHT_INFO("Web ... enable {} proto, listen on {}, route /{}/",
              config.Ssl.Enable ? "https" : "http",
              std::string {config.Tcp.HostPort()}.c_str(),
              config.Path.c_str());
}

capiserver::ctcpapiserver::~ctcpapiserver() {}

ssize_t capiserver::cunixapiserver::OnUnixAccept(fd_t fd, const sockaddr_storage& sa, const inet::ssl_t& ssl, const std::weak_ptr<inet::cpoll>& poll) {
    ssize_t res {-1};
    if (auto&& self {poll.lock()}; self) {
        return self->PollAdd(fd, EPOLLIN | EPOLLRDHUP | EPOLLERR, [this, sa, ssl, poll](auto&& PH1, auto&& PH2) {
            OnHttpData(std::forward<decltype(PH1)>(PH1), std::forward<decltype(PH2)>(PH2), sa, ssl, poll);
        });
    }
    return res;
}

void capiserver::cunixapiserver::OnHttpError(const inet::socket_t& fd, ssize_t err) {
    PSHT_ERROR("Unix server request error ( {}sss )", std::strerror(-(int)err));
    fd->SocketClose();
}

void capiserver::cunixapiserver::OnHttpData(fd_t fd, [[maybe_unused]] uint events, const sockaddr_storage& sa, const inet::ssl_t& ssl, const std::weak_ptr<inet::cpoll>& poll) {
    ssize_t res {-1};
    if (auto&& self {poll.lock()}; self) {
        std::string_view method, content;
        http::uri_t path;
        http::headers_t headers;
        std::string request;
        auto&& so = std::make_shared<inet::csocket>(fd, sa, ssl, poll);
        if (res = HttpReadRequest(*so, method, path, headers, request, content, HttpMaxHeaderSize); res == 0) {
            OnHttpRequest(so, method, path, headers, request, content);
        } else {
            OnHttpError(so, res);
        }
        return;
    }
    inet::Close(fd);
}

capiserver::cunixapiserver::cunixapiserver(capiserver& api, config::interface_t config, size_t httpMaxHeaderSize) :
    inet::cunixserver {"uapi:srv", config.Threads, config.Accept, false, 30},
    Api {api},
    HttpMaxHeaderSize {httpMaxHeaderSize} {
    try {
        std::filesystem::path saFile {config.Unix.Path()};

        std::filesystem::create_directories(saFile.parent_path());

        UnixListen(config.Unix.Path(), 16, AF_UNIX);
        PSHT_INFO("Unix ... enable, listen on {}, route /{}/", saFile.c_str(), config.Path.c_str());

    } catch (std::exception& ex) {
        PSHT_ERROR("Unix API Initialize server error ( {} )", ex.what());
        throw std::runtime_error("Unix API server exception");
    }
}

capiserver::cunixapiserver::~cunixapiserver() {}
