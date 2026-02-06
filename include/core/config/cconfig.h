#pragma once

#include <sys/types.h>

#include <cstddef>
#include <ctime>
#include <filesystem>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_set>
#include <variant>

#include "core/util/cflag.h"
#include "core/util/cyaml.h"
#include "inet/cinet.h"

namespace core {
class cconfig {
    enum class protocols_t { none = 0, pusher = 1, websocket = 2, mqtt3 = 4 };
    enum class api_driver_t { disable = 0, pusher = 1, push1st = 2 };
    enum class channels_t { none = 0, pub = 1, priv = 2, pres = 4, prot = 8 };
    enum class hook_trigger_t { none = 0, reg = 1, unreg = 2, join = 4, leave = 8, push = 16 };
    using cluster_sync_t = hook_trigger_t;

    class cdsn {
       public:
        cdsn() {
            ;
        }
        cdsn(const std::string& dsn) {
            Assign(dsn);
        }
        cdsn& operator=(const std::string& dsn) {
            Assign(dsn);
            return *this;
        }
        cdsn& operator=(const yaml_t& dsn) {
            if (dsn.IsDefined() and dsn.IsScalar()) {
                Assign(dsn.as<std::string>());
            }
            return *this;
        }
        bool Assign(const std::string& dsn);
        inline bool Empty() const {
            return Value.empty();
        }
        inline const std::string& Str() const {
            return Value;
        }
        inline const std::string_view& Proto() const {
            return Proto_;
        }
        inline const std::string_view& User() const {
            return User_;
        }
        inline const std::string_view& Pwd() const {
            return Pwd_;
        }
        inline const std::string_view& HostPort() const {
            return HostPort_;
        }
        inline const std::string_view& Host() const {
            return Host_;
        }
        inline const std::string_view& Port(const std::string_view& def = {}) const {
            return !Port_.empty() ? Port_ : def;
        }
        inline const std::string_view& Url() const {
            return Url_;
        }
        inline const std::string_view& Path() const {
            return Path_;
        }
        inline const std::string_view& Args() const {
            return Args_;
        }
        inline std::string Baseurl() const {
            std::string url {Proto_};
            url.append(HostPort_).append(Path_).append(Path_.empty() or Path_.back() != '/' ? "/" : "");
            return url;
        }
        inline bool IsWeb() const {
            return strncasecmp(Proto_.data(), "http://", 7) == 0 or strncasecmp(Proto_.data(), "https://", 8) == 0;
        }
        inline bool IsHttp() const {
            return strncasecmp(Proto_.data(), "http://", 7) == 0;
        }
        inline bool IsHttps() const {
            return strncasecmp(Proto_.data(), "https://", 8) == 0;
        }

       private:
        std::string Value;
        std::string_view Proto_, User_, Pwd_, HostPort_, Host_, Port_, Url_, Path_, Args_;
    };

   public:
    cconfig() {
        ;
    }
    ~cconfig() {
        ;
    }
    void Load(std::filesystem::path configfile);

   public:
    using proto_t = core::ci::cflag<core::cconfig::protocols_t>;
    using sync_t = core::ci::cflag<core::cconfig::cluster_sync_t>;
    using api_t = core::ci::cflag<core::cconfig::api_driver_t>;
    using channel_t = core::ci::cflag<core::cconfig::channels_t>;
    using hook_t = core::ci::cflag<core::cconfig::hook_trigger_t>;
    using dsn_t = cdsn;

    std::filesystem::path Path;
    struct ssloptions_t {
        bool Enable {false};
        bool Client {false};
        std::string Cert, Key;
        inet::ssl_ctx_t Context() const;
    };
    struct server_t {
        proto_t Proto {proto_t::type::none};
        size_t Threads {3};
        size_t Accept {3};
        cdsn Listen;
        ssloptions_t Ssl;
        std::string Path;
        struct srvproto_t {
            std::string Path;
            std::time_t ActivityTimeout {20};
            size_t MaxPayloadSize {65400};
            channel_t PushOn;
        } Pusher, WebSocket, MQTT;

       private:
        friend class cconfig;
        void Load(const std::filesystem::path& path, const yaml_t& options);
    } Server;
    struct cluster_t {
        enum class type_t { nil = -1, peers = 0, k8s };
        static type_t FromStr(const std::string_view& str);

        bool Enable {false};
        sync_t Sync {sync_t::type::none};
        std::time_t PingInterval {30};
        cdsn Listen;
        cdsn Module;
        type_t Type = type_t::nil;
        std::unordered_set<std::string> Nodes;
        std::string Url, Namespace;
        ssloptions_t Ssl;

       private:
        friend class cconfig;
        void Load(const std::filesystem::path& path, const yaml_t& options);
    } Cluster;
    struct interface_t {
        api_t Interface {api_t::type::disable};
        cdsn Tcp, Unix;
        std::string Path;
        std::time_t KeepAlive {10};
        ssloptions_t Ssl;
        size_t Threads {3};
        size_t Accept {3};

        struct smpp_t {
            bool Enable {false};
            std::string Hook;
            std::string Path;
            ssize_t Rps {100};

           private:
            friend class cconfig;
            void Load(const std::filesystem::path& path, const yaml_t& options);
        } Smpp;

       private:
        friend class cconfig;
        void Load(const std::filesystem::path& path, const yaml_t& options);
    } Api;

    struct credential_t {
        bool Enable {true};
        std::string Id, Name, Key, Secret;
        channel_t Channels {channel_t::type::none};
        hook_t Hooks {hook_t::type::none};
        bool OptionClientMessages {false}, OptionStatistic {false}, OptionKeepAlive {false};
        std::unordered_set<std::string> Origins;
        std::unordered_set<std::string> Endpoints;
        credential_t() {
            ;
        }
        credential_t(const credential_t& cred);

       private:
        friend class cconfig;
        void Load(const std::filesystem::path& path);
    };
    struct credentials_t {
        std::vector<credential_t> Apps;

       private:
        friend class cconfig;
        void Load(const std::filesystem::path& basedir, const yaml_t& options);
    } Credentials;

    struct clogger_t {
        struct cstdout_t {
            std::string tag;
            int verbose;
            std::string pattern;
        };

        struct cfile_t {
            std::string tag;
            int number;
            int rotate;
            int verbose;
            std::string path;
            std::string pattern;
        };

        struct cringbuffer_t {
            std::string tag;
            int number;
            int verbose;
            std::string pattern;
        };

        struct csyslog_t {
            std::string tag;
            int verbose;
            std::string facility;
            std::string pattern;
        };

        struct cremote_t {
            std::string tag;
            int verbose;
            std::string url;
            std::string pattern;
        };

        std::optional<cstdout_t> stdoutSink;
        std::optional<cfile_t> fileSink;
        std::optional<csyslog_t> syslogSink;
        std::optional<cremote_t> remoteSink;

       private:
        friend class cconfig;
        void Load(const std::filesystem::path& basedir, const yaml_t& options);

        template <class T>
        static void BaseInit(T& sink, const yaml_t& cfg) {
            if (auto&& tag = cfg["tag"]; tag and tag.IsScalar()) {
                sink->tag = tag.as<std::string>();
            }
            if (auto&& pattern = cfg["pattern"]; pattern and pattern.IsScalar()) {
                sink->pattern = pattern.as<std::string>();
            }
            if (auto&& verbose = cfg["verbose"]; verbose and verbose.IsScalar()) {
                sink->verbose = verbose.as<int>();
            }
        }

    } Logger;
};
}// namespace core

namespace config {
using instance_t = const core::cconfig::server_t&;
using cluster_t = const core::cconfig::cluster_t&;
using server_t = const core::cconfig::server_t&;
using interface_t = const core::cconfig::interface_t&;
using credentials_t = const core::cconfig::credentials_t&;
using credential_t = core::cconfig::credential_t;
}// namespace config

using proto_t = core::cconfig::proto_t;
using sync_t = core::cconfig::sync_t;
using api_t = core::cconfig::api_t;
using channel_t = core::cconfig::channel_t;
using hook_t = core::cconfig::hook_t;
using dsn_t = core::cconfig::dsn_t;

namespace cluster {
using type_t = core::cconfig::cluster_t::type_t;
}// namespace cluster
