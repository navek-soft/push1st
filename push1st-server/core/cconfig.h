#pragma once
#include <string>
#include <filesystem>
#include <unordered_map>
#include <unordered_set>
#include "ci/cflag.h"
#include "ci/cyaml.h"
#include "../inet/cinet.h"
#include <ctime>

namespace core {
	class cconfig {
		enum class protocols_t { none = 0, pusher = 1, websocket = 2, mqtt3 = 4 };
		enum class cluster_sync_t { none = 0, session = 1, stat = 2, health = 4};
		enum class api_driver_t { disable = 0, pusher = 1, push1st = 2 };
		enum class channels_t { none = 0, pub = 1, priv = 2, pres = 4, prot = 8 };
		enum class hook_trigger_t { none = 0, reg = 1, unreg = 2, join = 4, leave = 8, push = 16 };

		class cdsn {
		public:
			cdsn() { ; }
			cdsn(const std::string& dsn) { assign(dsn); }
			cdsn& operator = (const std::string& dsn) { assign(dsn); return *this; }
			cdsn& operator = (const yaml_t& dsn) { if (dsn.IsDefined() and dsn.IsScalar()) { assign(dsn.as<std::string>()); } return *this; }
			bool assign(const std::string& dsn);
			inline bool empty() const { return Value.empty(); }
			inline const std::string& str() const { return Value; }
			inline const std::string_view& proto() const { return Proto; }
			inline const std::string_view& user() const { return User; }
			inline const std::string_view& pwd() const { return Pwd; }
			inline const std::string_view& hostport() const { return HostPort; }
			inline const std::string_view& host() const { return Host; }
			inline const std::string_view& port(const std::string_view& def = {}) const { return !Port.empty() ? Port : def; }
			inline const std::string_view& url() const { return Url; }
			inline const std::string_view& path() const { return Path; }
			inline const std::string_view& args() const { return Args; }
			inline std::string baseurl() const { std::string url{ Proto }; url.append(HostPort).append(Path).append(Path.empty() or Path.back() != '/' ? "/" : ""); return url; }
		private:
			std::string Value;
			std::string_view Proto, User, Pwd, HostPort, Host, Port, Url, Path, Args;
		};
	public:
		cconfig() { ; }
		~cconfig() { ; }
		void Load(const std::filesystem::path& configfile);
	public:
		using proto_t = core::ci::cflag<core::cconfig::protocols_t>;
		using sync_t = core::ci::cflag<core::cconfig::cluster_sync_t>;
		using api_t = core::ci::cflag<core::cconfig::api_driver_t>;
		using channel_t = core::ci::cflag<core::cconfig::channels_t>;
		using hook_t = core::ci::cflag<core::cconfig::hook_trigger_t>;
		using dsn_t = cdsn;

		std::filesystem::path Path;

		struct instance_t {
			proto_t Proto{ proto_t::type::none };
			size_t Threads{ 3 };
			size_t MaxPayloadSize{ 65400 };
		private:
			friend class cconfig;
			void load(const std::filesystem::path& path, const yaml_t& options);
		} Server;
		struct cluster_t {
			bool Enable{ false };
			ssize_t Node;
			sync_t Sync{ sync_t::type::none };
			cdsn Listen;
			std::unordered_map<ssize_t, std::string> Nodes;
		private:
			friend class cconfig;
			void load(const std::filesystem::path& path, const yaml_t& options);
		} Cluster;
		struct server_t {
			cdsn Listen;
			std::string Path;
			std::time_t ActivityTimeout{ 20 };
			size_t MaxPayloadSize{ 8192 };
			struct ssloptions_t {
				bool Enable{ false };
				std::string Cert, Key;
				inet::ssl_ctx_t Context() const;
			} Ssl;
		private:
			friend class cconfig;
			void load(const std::filesystem::path& path, const yaml_t& options, size_t MaxPayloadSize);
		} Pusher, WebSocket;
		struct interface_t : public server_t {
			api_t Interface{ api_t::type::disable };
			std::vector<server_t> Listen;
			std::time_t KeepAlive{ 10 };
		private:
			friend class cconfig;
			void load(const std::filesystem::path& path, const yaml_t& options);
		} Api;

		struct credential_t {
			std::string Id, Name, Key, Secret;
			channel_t Channels{ channel_t::type::none };
			bool OptionClientMessages{ false }, OptionStatistic{ false };
			std::unordered_set<std::string> Origins;
			std::unordered_multimap<hook_trigger_t,std::pair<std::string, std::string>> Hooks;
			credential_t() { ; }
			credential_t(const credential_t& cred);
		private:
			friend class cconfig;
			void load(const std::filesystem::path& path);
		};
		struct credentials_t {
			std::vector<credential_t> Apps;
		private:
			friend class cconfig;
			void load(const std::filesystem::path& path, const yaml_t& options);
		} Credentials;
	};
}

namespace config {
	using instance_t = const core::cconfig::server_t&;
	using cluster_t = const core::cconfig::cluster_t&;
	using server_t = const core::cconfig::server_t&;
	using interface_t = const core::cconfig::interface_t&;
	using credentials_t = const core::cconfig::credentials_t&;
	using credential_t = core::cconfig::credential_t;
}

using proto_t = core::cconfig::proto_t;
using sync_t = core::cconfig::sync_t;
using api_t = core::cconfig::api_t;
using channel_t = core::cconfig::channel_t;
using hook_t = core::cconfig::hook_t;
using dsn_t = core::cconfig::dsn_t;
