#include "core/cluster/adapters/ck8sadapter.h"

#include <core/util/cjson.h>
#include <http/cresponse.h>
#include <netinet/in.h>
#include <sys/types.h>

#include <cstddef>
#include <cstdint>
#include <mutex>
#include <stdexcept>
#include <string>
#include <tuple>
#include <vector>

#include "http/chttp.h"
#include "http/chttprequest.h"
#include "inet/ciface.h"
#include "inet/cinet.h"
#include "log/clog.h"

void ck8sadapter::Start() {
    Init();

    k8sPoll->Listen();
    Connect();

    if (not k8sSocket) {
        PSHT_WARNING("can't connect to k8s {}", inet::GetAddress(sa));
    }
};

void ck8sadapter::Check() {
    switch (status) {
        case status_t::disconnected: {
            Connect();
            break;
        }
        default:
            break;
    }
}

void ck8sadapter::Disconnect() {
    k8sSocket.SocketClose();
    status = status_t::disconnected;
}

void ck8sadapter::Init() {
    const std::string podPath = fmt::format("/api/v1/namespaces/{}/pods/{}", space, pod);

    std::unordered_map<std::string_view, std::string> headers;
    headers.emplace("Content-Type", "application/json");
    headers.emplace("Host", url);
    headers.emplace("Authorization", fmt::format("Bearer {}", token));

    http::crequest req {url, std::move(headers), sslCtx};
    auto&& resp = req.Call(podPath);
    if (resp.Code() != "200") {
        throw std::runtime_error(fmt::format("can't get k8s pod info {}", resp.Payload()));
    }

    auto&& data = resp.Json();
    if (not data.contains("metadata") or not data["metadata"].is_object()) {
        throw std::runtime_error("unexpected k8s pod info");
    }

    auto&& metadata = data["metadata"];
    if (not metadata.contains("ownerReferences") or not metadata["ownerReferences"].is_array()) {
        throw std::runtime_error("unexpected k8s pod info");
    }

    std::string statefulSetName {};
    for (auto&& ref : metadata["ownerReferences"]) {
        if (http::ToLower(ref["kind"].get<std::string>()) != "statefulset") {
            continue;
        }
        statefulSetName = ref["name"].get<std::string>();
        break;
    }

    const std::string statefulsetPath = fmt::format("/apis/apps/v1/namespaces/{}/statefulsets/{}", space, statefulSetName);
    auto&& ssResp = req.Call(statefulsetPath);
    if (ssResp.Code() != "200") {
        throw std::runtime_error(fmt::format("can't get k8s statefulset info {}", ssResp.Payload()));
    }

    auto&& ssData = ssResp.Json();

    if (not ssData.contains("spec") or not ssData["spec"].is_object()) {
        throw std::runtime_error("unexpected k8s statefulset info");
    }

    auto&& spec = ssData["spec"];
    if (not spec.contains("serviceName") or not spec["serviceName"].is_string()) {
        throw std::runtime_error("unexpected k8s statefulset info");
    }

    serviceName = ssData["spec"]["serviceName"].get<std::string>();
    PSHT_DEBUG("k8s got serviceName: {}", serviceName)
}

void ck8sadapter::Connect() {
    k8sSocket.SocketClose();

    inet::ssl_t fdSsl;
    fd_t fd {-1};

    if (ssize_t res; sslCtx) {
        if (res = inet::SslConnect(fd, sa, false, 1500, sslCtx, fdSsl); res != 0) {
            PSHT_ERROR("Ssl connect to k8s {}  ... error ( {}:{} )", inet::GetAddress(sa), std::strerror(-(int)res), inet::SslGetErrorString());
            Disconnect();
            return;
        }
    } else if (res = inet::TcpConnect(fd, sa, false, 10000); res != 0) {
        PSHT_ERROR("Connect to k8s {}  ... error ( {} )", inet::GetAddress(sa), std::strerror(-(int)res));
        Disconnect();
        return;
    }

    k8sSocket = inet::csocket {fd, sa, sslCtx ? fdSsl : inet::ssl_t {}, k8sPoll};
    k8sSocket.SetKeepAlive(true, 3, 10, 3);
    k8sSocket.SetRecvTimeout(3000);
    k8sSocket.SetSendTimeout(1000);

    if (auto&& res = k8sSocket.Poll()->PollAdd(k8sSocket.Fd(),
                                               EPOLLIN,
                                               [this](auto&& PH1, auto&& PH2) {
                                                   OnK8sReply(std::forward<decltype(PH1)>(PH1), std::forward<decltype(PH2)>(PH2));
                                               });
        res < 0) {
        PSHT_ERROR("Can't add {} fd to epoll {}", k8sSocket.Fd(), std::strerror(-res));
        Disconnect();
        return;
    }

    status = status_t::started;
    PSHT_INFO("Connect to k8s {}  ... success", inet::GetAddress(sa));

    std::unordered_map<std::string_view, std::string> headers;
    headers.emplace("Content-Type", "application/json");
    headers.emplace("Host", url);
    headers.emplace("Authorization", fmt::format("Bearer {}", token));

    static const std::string path =
        fmt::format("/apis/discovery.k8s.io/v1/namespaces/{}/endpointslices?watch=true{}", space, not serviceName.empty() ? fmt::format("&labelSelector=kubernetes.io/service-name\%3D{}", serviceName) : "");

    // static const std::string path = fmt::format("/apis/discovery.k8s.io/v1/namespaces/{}/endpointslices?watch=true", space);
    PSHT_DEBUG("k8s api path {}", path);

    if (auto&& res = HttpWriteRequest(k8sSocket, "GET", path, std::move(headers), ""); res < 0) {
        PSHT_ERROR("Send '{}' to k8s {}  ... error ( {} )", path, inet::GetAddress(sa), std::strerror(-(int)res));
        Disconnect();
        return;
    }
}

void ck8sadapter::OnK8sReply([[maybe_unused]] fd_t fd, uint events) {
    constexpr size_t max_size = 65536;

    if (k8sSocket) {
        if (events == EPOLLIN) {
            switch (status) {
                case status_t::started: {
                    std::string response;
                    std::string_view code;
                    std::string_view msg;
                    http::headers_t headers;
                    std::string_view content;

                    if (auto&& res = HttpReadResponse(k8sSocket, code, msg, headers, response, content, max_size); res >= 0) {
                        auto&& resp = http::response_t {std::move(response), code, msg, std::move(headers), std::string {content}};
                        status = status_t::active;
                        OnResponse(std::move(resp));
                        return;
                    } else {
                        PSHT_ERROR("K8s reply error: {} - {}", std::strerror(-(int)res), response);
                    }
                    break;
                }
                case status_t::active: {
                    std::vector<http::chunk_t> chunks;
                    if (auto&& res = HttpReadChunkedResponse(k8sSocket, chunks, max_size); res == 0 or res == -ENODATA) {
                        for (auto& chunk : chunks) {
                            OnResponse(std::move(chunk));
                        }
                        return;
                    } else {
                        PSHT_ERROR("K8s chunked reply error: {}", std::strerror(-(int)res));
                    }
                    break;
                }
                default:
                    break;
            };
        }

        // reconnect on error
        Disconnect();
    }
}

void ck8sadapter::OnResponse(http::chunk_t&& resp) {
    OnResponse(resp.Json());
}

void ck8sadapter::OnResponse(http::response_t&& resp) {
    OnResponse(resp.Json());
}

void ck8sadapter::OnResponse(json::value_t&& jsonResp) {
    if (jsonResp.empty()) {
        return;
    }

    k8sevent_t type = k8sevent_t::none;

    std::vector<std::tuple<std::string, uint32_t, bool>> endpoints;

    const auto&& processItem = [&endpoints](const json::value_t& item) {
        if (not item.contains("endpoints") or item["endpoints"].empty() or not item.contains("ports") or item["ports"].empty()) {
            return;
        }

        for (const auto& endpoint : item["endpoints"]) {
            auto&& addr = endpoint["addresses"].begin()->get<std::string>();
            int port = -1;
            for (auto&& portObj : item["ports"]) {
                if (not portObj.contains("protocol") or not portObj.contains("name") or not portObj.contains("port")) {
                    continue;
                }

                if (http::ToLower(portObj["protocol"].get<std::string>()) != "udp") {
                    continue;
                }

                if (http::ToLower(portObj["name"].get<std::string>()) != "cluster") {
                    continue;
                }

                port = portObj["port"].get<int>();
                break;
            }

            if (port == -1) {
                continue;
            }

            bool ready = false;
            const auto& conditions = endpoint["conditions"];
            if (auto&& it = conditions.find("ready"); it != conditions.end()) {
                ready = it->get<bool>();
            }

            /// TODO: maybe need to add some checks to identify push1st pod
            endpoints.emplace_back(addr, port, ready);
        }
    };

    const auto&& validate = [](const json::value_t& object) -> bool {
        if (auto&& it = object.find("kind"); it == object.end() or not it->is_string()) {
            PSHT_ERROR("wrong k8s response {}", object.dump(-1));
            return false;
        } else if (auto&& kind = it->get<std::string>(); kind.find("EndpointSlice") == std::string::npos) {
            PSHT_ERROR("wrong k8s response `kind `{}", kind);
            return false;
        }
        return true;
    };

    if (auto&& it = jsonResp.find("type"); it == jsonResp.end()) {
        PSHT_ERROR("k8s is not getting updates {}", jsonResp.dump(-1));
        return;
    } else {
        if (auto&& strType = it->get<std::string>(); (type = FromStr(strType)) == k8sevent_t::none) {
            PSHT_ERROR("got wront update type: {}", strType);
            return;
        }

        if (auto&& object = jsonResp["object"]; validate(object)) {
            processItem(object);
        } else {
            return;
        }
    }

    if (endpoints.empty()) {
        return;
    }

    std::unique_lock _ {clusLock};
    switch (type) {
        case k8sevent_t::added:
        case k8sevent_t::modified: {
            for (const auto& [host, port, ready] : endpoints) {
                sockaddr_storage peerSa;
                if (inet::GetSockAddr(peerSa, host, std::to_string(port), AF_INET) == 0) {
                    auto&& nodeIp = inet::GetIp4(peerSa);
                    if (ready) {
                        auto&& node = std::make_unique<node_t>(nodeIp, peerSa);
                        if (not inet::IsLocalIp(node->NodeIp)) {
                            PSHT_INFO("Node {}:{} ... enable", host, port);
                            clusNodes.emplace(node->NodeIp, std::move(node));
                        } else {
                            PSHT_INFO("Node {}:{} ... I'm, address", host, port);
                        }
                    } else {
                        if (clusNodes.erase(nodeIp)) {
                            PSHT_INFO("Node {}:{} ... deleted", host, port);
                        } else {
                            PSHT_INFO("Node {}:{} ... not found to delete", host, port);
                        }
                    }
                }
            }
            break;
        }
        case k8sevent_t::deleted:
            for (const auto& [host, port, _] : endpoints) {
                sockaddr_storage peerSa;
                if (inet::GetSockAddr(peerSa, host, std::to_string(port), AF_INET) == 0) {
                    auto&& nodeIp = inet::GetIp4(peerSa);
                    if (clusNodes.erase(nodeIp)) {
                        PSHT_INFO("Node {}:{} ... deleted", host, port);
                    } else {
                        PSHT_INFO("Node {}:{} ... not found to delete", host, port);
                    }
                }
            }
            break;
        default:
            break;
    }
}

ck8sadapter::ck8sadapter(const std::shared_ptr<inet::cpoll>& poll, const std::string_view& url, const std::string_view& space, const std::string_view& token, const std::string_view& podName, const inet::ssl_ctx_t& ssl) :
    k8sSocket {-1, {}},
    k8sPoll {poll},
    sslCtx {ssl},
    url {url},
    space {space},
    token {token},
    pod {podName} {
    if (auto&& res = inet::GetSockAddr(*const_cast<sockaddr_storage*>(&sa), url, "0", AF_INET); res < 0) {
        throw std::runtime_error(fmt::format("can't get k8s address on {} error {}:{}", url, -res, std::strerror(-(int)res)));
    }
}

ck8sadapter::~ck8sadapter() {
    Disconnect();
    k8sPoll->Join();
}
