#pragma once

#include <unistd.h>

#include <cstdint>
#include <string>
#include <string_view>

#include "cadapter.h"
#include "core/config/cconfig.h"
#include "core/util/cfactory.h"
#include "http/chttpconn.h"

enum class k8sevent_t { none = -1, added = 0, modified, deleted };

inline k8sevent_t FromStr(const std::string_view& str) {
    if (str == "ADDED") {
        return k8sevent_t::added;
    }

    if (str == "MODIFIED") {
        return k8sevent_t::modified;
    }

    if (str == "DELETED") {
        return k8sevent_t::deleted;
    }

    return k8sevent_t::none;
}

class ck8sadapter : public cadapter, public inet::chttpconnection, public cuniquefactory<ck8sadapter> {
    log_as(k8sadapter);
    friend cuniquefactory<ck8sadapter>;

    enum class status_t { none = -1, started = 0, active, disconnected };

   protected:
    ck8sadapter(const std::shared_ptr<inet::cpoll>& poll, const std::string_view& url, const std::string_view& space, const inet::ssl_ctx_t& ssl);

   public:
    ~ck8sadapter() override;

   public:
    void Start() override;
    void Check() override;

   private:
    void Connect();

    void OnK8sReply(fd_t, uint);

    void OnResponse(http::response_t&& resp) override;
    void OnResponse(http::chunk_t&&) override;
    void OnResponse(json::value_t&&);

    void Disconnect();

   private:
    inet::csocket k8sSocket;
    std::shared_ptr<inet::cpoll> k8sPoll;
    inet::ssl_ctx_t sslCtx;
    const sockaddr_storage sa {};
    std::string url, space;

    std::atomic<status_t> status {status_t::none};
};