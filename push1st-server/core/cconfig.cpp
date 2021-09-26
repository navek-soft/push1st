#include "cconfig.h"
#include "csyslog.h"
#include <regex>
#include <fnmatch.h>

using namespace core;

template<typename ET>
inline flag_t<ET> Map(const yaml_t& vals, const std::unordered_map<std::string_view, ET>&& names, ET def) {
	flag_t<ET> res{ def };
	if (vals.IsDefined() and vals.IsSequence()) {
		for (auto&& val : vals) {
			if (auto&& it{ names.find(val.as<std::string>()) }; it != names.end()) {
				res |= it->second;
			}
		}
	}
	return res;
}

template<typename T>
inline T Value(const yaml_t& vals, T def) {
	if (vals.IsDefined() and vals.IsScalar()) {
		return vals.as<T>();
	}
	return def;
}

void cconfig::instance_t::load(const std::filesystem::path& path, const yaml_t& options) {
	if (options.IsDefined() and options.IsMap()) {
		Proto = Map(options["proto"], { {"pusher",proto_t::type::pusher}, {"mqtt3",proto_t::type::mqtt3}, {"websocket",proto_t::type::websocket} }, proto_t::type::none);
		Threads = Value<size_t>(options["threads"], Threads);
		MaxPayloadSize = Value<size_t>(options["max-request-payload"], MaxPayloadSize);
	}
	else {
		throw std::runtime_error{"Server section not found"};
	}
}

void cconfig::cluster_t::load(const std::filesystem::path& path, const yaml_t& options) {
	if (options.IsDefined() and options.IsMap()) {
		Listen = options["listen"];
		Node = Value<ssize_t>(options["node"], -1);
		Sync = Map(options["sync"], { {"session",sync_t::type::session}, {"stat",sync_t::type::stat}, {"health",sync_t::type::health} }, sync_t::type::none);

		if (options["family"].IsSequence()) {
			for (auto&& node : options["family"]) {
				if (node.IsMap()) {
					for (auto&& host : node) {
						if (host.first.IsScalar() and host.second.IsScalar()) {
							Nodes.emplace(Value<ssize_t>(host.first, 0), Value<std::string>(host.second, {}));
						}
					}
				}
			}
		}
		Enable = !Listen.empty() and Node > 0;
	}
	else {
		Enable = false;
	}
}

inet::ssl_ctx_t cconfig::server_t::ssloptions_t::Context() const {
	//inet::SslCreateContext(sslCtx, SslCert, SslKey, true)
	return {};
}

void cconfig::server_t::load(const std::filesystem::path& path, const yaml_t& options, size_t MaxPayloadSize) {
	if (options.IsDefined() and options.IsMap()) {
		std::string_view trimPath{ Value<std::string>(options["path"], {"app"})};
		while (!trimPath.empty() and trimPath.front() == '/') trimPath.remove_prefix(1);
		while (!trimPath.empty() and trimPath.back() == '/') trimPath.remove_suffix(1);
		Listen = options["listen"];
		Path = trimPath;
		ActivityTimeout = Value<std::time_t>(options["activity-timeout"], ActivityTimeout);
		Ssl.Cert = Value<std::string>(options["ssl"]["cert"], {});
		Ssl.Key = Value<std::string>(options["ssl"]["key"], {});
		Ssl.Enable = Value<bool>(options["ssl"]["enable"], false) and !Ssl.Cert.empty() and !Ssl.Key.empty();
	}
}

void cconfig::interface_t::load(const std::filesystem::path& path, const yaml_t& options) {
	KeepAlive = Value<std::time_t>(options["keep-alive-timeout"], 0);
	Interface = Map(options["interface"], { {"pusher",api_t::type::pusher},{"push1st",api_t::type::push1st} }, api_t::type::disable);
	std::string Path{ "apps" };
	ssloptions_t Ssl;

	if (auto path = Value<std::string>(options["path"], Path);  1) {
		std::string_view trimPath{ path };
		while (!trimPath.empty() and trimPath.front() == '/') trimPath.remove_prefix(1);
		while (!trimPath.empty() and trimPath.back() == '/') trimPath.remove_suffix(1);
		
		if (!trimPath.empty()) {
			Path = trimPath;
		}
	}
	Ssl.Cert = Value<std::string>(options["ssl"]["cert"], {});
	Ssl.Key = Value<std::string>(options["ssl"]["key"], {});
	Ssl.Enable = Value<bool>(options["ssl"]["enable"], false) and !Ssl.Cert.empty() and !Ssl.Key.empty();

	if (options["listen"].IsDefined() and options["listen"].IsSequence()) {
		for (auto&& dsn : options["listen"]) {
			Listen.push_back({});
			Listen.back().Listen = dsn;
			Listen.back().Path = Path;
			Listen.back().Ssl.Enable = Ssl.Enable;
			Listen.back().Ssl.Cert = Ssl.Cert;
			Listen.back().Ssl.Key = Ssl.Key;
		}
	}
	else {
		Interface = api_t::type::disable;
	}
}

cconfig::credential_t::credential_t(const credential_t& cred) :
	Id{ std::move(cred.Id) }, Name{ std::move(cred.Name) }, Key{ std::move(cred.Key) }, Secret{ std::move(cred.Secret) },
	Channels{ cred.Channels }, OptionClientMessages{ cred.OptionClientMessages }, OptionStatistic{ cred.OptionStatistic },
	Origins{ std::move(cred.Origins) }, Hooks{ std::move(cred.Hooks) }
{
	;
}

void cconfig::credential_t::load(const std::filesystem::path& path) {
	auto&& credOptions{ yaml::load(path) };
	if (credOptions.IsDefined() and credOptions.IsMap()) {
		for (auto&& cred : credOptions) {
			Id = Value<std::string>(cred.first, {});
			Name = Value<std::string>(cred.second["name"], {});
			Key = Value<std::string>(cred.second["key"], {});
			Secret = Value<std::string>(cred.second["secret"], {});
			OptionClientMessages = Value<bool>(cred.second["options"]["client-messages"], OptionClientMessages);
			OptionStatistic = Value<bool>(cred.second["options"]["statistic"], OptionStatistic);
			Channels = Map(cred.second["channels"], { {"public", channel_t::type::pub},{"private",channel_t::type::priv},{"presence",channel_t::type::pres},{"protected",channel_t::type::prot} }, channel_t::type::none);

			if (cred.second["origins"].IsDefined() and cred.second["origins"].IsSequence()) {
				for (auto&& orig : cred.second["origins"]) { Origins.emplace(orig.as<std::string>()); }
			}

			if (cred.second["hook"].IsDefined()) {
				if (cred.second["hook"]["register"].IsSequence()) {
					for (auto&& item : cred.second["hook"]["register"]) {
						if (item.IsMap()) {
							for (auto&& hook : item) {
								if (auto endpoint{ hook.second.as<std::string>() }; !endpoint.empty() and 
									(endpoint.compare(0, 7, "http://") == 0 or endpoint.compare(0, 8, "https://") == 0 or endpoint.compare(0, 6, "lua://") == 0))
								{
									Hooks.emplace(hook_t::type::reg, std::make_pair(hook.first.as<std::string>(), endpoint));
								}
							}
						}
					}
					for (auto&& item : cred.second["hook"]["unregister"]) {
						if (item.IsMap()) {
							for (auto&& hook : item) {
								if (auto endpoint{ hook.second.as<std::string>() }; !endpoint.empty() and
									(endpoint.compare(0, 7, "http://") == 0 or endpoint.compare(0, 8, "https://") == 0 or endpoint.compare(0, 6, "lua://") == 0))
								{
									Hooks.emplace(hook_t::type::unreg, std::make_pair(hook.first.as<std::string>(), endpoint));
								}
							}
						}
					}
					for (auto&& item : cred.second["hook"]["join"]) {
						if (item.IsMap()) {
							for (auto&& hook : item) {
								if (auto endpoint{ hook.second.as<std::string>() }; !endpoint.empty() and
									(endpoint.compare(0, 7, "http://") == 0 or endpoint.compare(0, 8, "https://") == 0 or endpoint.compare(0, 6, "lua://") == 0))
								{
									Hooks.emplace(hook_t::type::join, std::make_pair(hook.first.as<std::string>(), endpoint));
								}
							}
						}
					}
					for (auto&& item : cred.second["hook"]["leave"]) {
						if (item.IsMap()) {
							for (auto&& hook : item) {
								if (auto endpoint{ hook.second.as<std::string>() }; !endpoint.empty() and
									(endpoint.compare(0, 7, "http://") == 0 or endpoint.compare(0, 8, "https://") == 0 or endpoint.compare(0, 6, "lua://") == 0))
								{
									Hooks.emplace(hook_t::type::leave, std::make_pair(hook.first.as<std::string>(), endpoint));
								}
							}
						}
					}
					for (auto&& item : cred.second["hook"]["push"]) {
						if (item.IsMap()) {
							for (auto&& hook : item) {
								if (auto endpoint{ hook.second.as<std::string>() }; !endpoint.empty() and
									(endpoint.compare(0, 7, "http://") == 0 or endpoint.compare(0, 8, "https://") == 0 or endpoint.compare(0, 6, "lua://") == 0))
								{
									Hooks.emplace(hook_t::type::push, std::make_pair(hook.first.as<std::string>(), endpoint));
								}
							}
						}
					}
				}
			}
		}
	}
}

void cconfig::credentials_t::load(const std::filesystem::path& basedir, const yaml_t& options) {
	if (options.IsDefined() and options.IsSequence()) {

		for (auto&& req : options) {
			std::filesystem::path path(req.as<std::string>());
			std::string mask;
			if (!path.empty()) {
				if (path.is_relative()) { path = basedir / path; }
				mask = path.filename();

				for (const auto& entry : std::filesystem::directory_iterator(path.parent_path())) {
					if (entry.is_regular_file() and fnmatch(mask.c_str(), entry.path().c_str(), FNM_CASEFOLD) == 0) {
						Apps.push_back({});
						Apps.back().load(entry.path());
					}
				}
			}
		}
		/*credential_t cred;
		cred.Id = Value<std::string>(options.)
		*/
	}
}

void cconfig::Load(const std::filesystem::path& configfile) {
	syslog.ob.print("Config file", "%s", configfile.c_str());
	try {
		Path = configfile.parent_path();

		auto&& cfgOptions{ yaml::load(configfile) };

		Server.load(Path, cfgOptions["server"]);
		Cluster.load(Path, cfgOptions["cluster"]);
		Pusher.load(Path, cfgOptions["pusher"], Server.MaxPayloadSize);
		WebSocket.load(Path, cfgOptions["websocket"], Server.MaxPayloadSize);
		Api.load(Path, cfgOptions["api"]);
		Credentials.load(Path, cfgOptions["credentials"]);
	}
	catch (std::exception& ex) {
		syslog.exit("Load config file ( %s )\n\tException ... %s\n", configfile.c_str(), ex.what());
	}
}


bool cconfig::cdsn::assign(const std::string& dsn) {
	static const std::regex re(R"(^([\w-]+://)?(?:([^:@]+)(?:\:(.+))?@)?(([^:/]+)(?::(\d+))?)(([^?]*)(.*)))");
	Value.assign(dsn);
	std::string_view src{ Value };
	std::cmatch match;
	if (std::regex_match(src.begin(), src.end(), match, re)) {
		Proto = std::string_view{ match[1].first, (size_t)match[1].length() };
		User = std::string_view{ match[2].first,(size_t)match[2].length() };
		Pwd = std::string_view{ match[3].first,(size_t)match[3].length() };
		HostPort = std::string_view{ match[4].first,(size_t)match[4].length() };
		Host = std::string_view{ match[5].first,(size_t)match[5].length() };
		Port = std::string_view{ match[6].first,(size_t)match[6].length() };
		Url = std::string_view{ match[7].first,(size_t)match[7].length() };
		Path = std::string_view{ match[8].first,(size_t)match[8].length() };
		Args = std::string_view{ match[9].first,(size_t)match[9].length() };
		return true;
	}
	Value.clear();
	return false;
}