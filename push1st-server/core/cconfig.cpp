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

template<typename ET>
inline flag_t<ET> Map(const yaml_t& vals, const std::unordered_map<std::string_view, ET>&& names, uint def) {
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

static inline std::string PathValue(const yaml_t& vals, const std::string& def) {
	std::string val;
	if (vals.IsDefined() and vals.IsScalar() and !vals.IsNull()) {
		val = vals.as<std::string>();
	}
	std::string_view trimPath{ val };
	while (!trimPath.empty() and trimPath.front() == '/') trimPath.remove_prefix(1);
	while (!trimPath.empty() and trimPath.back() == '/') trimPath.remove_suffix(1);

	return !trimPath.empty() ? std::string{ trimPath } : def;
}

void cconfig::server_t::load(const std::filesystem::path& path, const yaml_t& options) {
	if (options.IsDefined() and options.IsMap()) {
		Proto = Map(options["proto"], { {"pusher",proto_t::type::pusher}, {"mqtt",proto_t::type::mqtt3}, {"websocket",proto_t::type::websocket} }, proto_t::type::none);
		Threads = Value<size_t>(options["threads"], Threads);
		Listen = options["listen"];
		Ssl.Cert = Value<std::string>(options["ssl"]["cert"], {});
		Ssl.Key = Value<std::string>(options["ssl"]["key"], {});
		Ssl.Enable = Value<bool>(options["ssl"]["enable"], false);

		Path = PathValue(options["path"], "app");

		if (options["pusher"].IsMap()) {
			Pusher.ActivityTimeout = (std::time_t)Value<size_t>(options["pusher"]["activity-timeout"], Pusher.ActivityTimeout);
			Pusher.MaxPayloadSize = Value<size_t>(options["pusher"]["max-request-payload"], Pusher.MaxPayloadSize);
			Pusher.PushOn = channel_t::type::pres;
			Pusher.Path = PathValue(options["pusher"]["path"], "pusher");
		}
		else {
			Proto -= proto_t::type::pusher;
		}

		if (options["websocket"].IsMap()) {
			WebSocket.ActivityTimeout = (std::time_t)Value<size_t>(options["websocket"]["activity-timeout"], WebSocket.ActivityTimeout);
			WebSocket.MaxPayloadSize = Value<size_t>(options["websocket"]["max-request-payload"], Pusher.MaxPayloadSize);
			WebSocket.PushOn = Map<channel_t::type>(options["websocket"]["push"], {
				{"public",channel_t::type::pub}, {"private",channel_t::type::priv}, {"presence", channel_t::type::pres}}, channel_t::type::pub | channel_t::type::prot | channel_t::type::pres);
			WebSocket.Path = PathValue(options["websocket"]["path"], "ws");
		}
		else {
			Proto -= proto_t::type::websocket;
		}

		if (options["mqtt"].IsMap()) {
			MQTT.ActivityTimeout = (std::time_t)Value<size_t>(options["mqtt"]["activity-timeout"], 0);
			MQTT.MaxPayloadSize = Value<size_t>(options["mqtt"]["max-request-payload"], Pusher.MaxPayloadSize);
			MQTT.PushOn = Map<channel_t::type>(options["mqtt"]["push"], {
				{"public",channel_t::type::pub}, {"private",channel_t::type::priv}, {"presence", channel_t::type::pres} }, channel_t::type::pub | channel_t::type::prot | channel_t::type::pres);
			MQTT.Path = PathValue(options["mqtt"]["path"], "mqtt");
		}
		else {
			Proto -= proto_t::type::mqtt3;
		}
	}
	else {
		throw std::runtime_error{"Server section not found"};
	}
}

void cconfig::cluster_t::load(const std::filesystem::path& path, const yaml_t& options) {
	if (options.IsDefined() and options.IsMap()) {
		Listen = options["listen"];
		Module = options["module"];
		//Node = Value<ssize_t>(options["node"], -1);
		PingInterval = (std::time_t)Value<size_t>(options["ping-interval"], PingInterval);
		Sync = Map(options["sync"], {
				{"register", sync_t::type::reg},{"unregister",sync_t::type::unreg},{"join",sync_t::type::join},{"leave",sync_t::type::leave},{"push",sync_t::type::push} }, sync_t::type::none);

		if (options["family"].IsSequence()) {
			for (auto&& node : options["family"]) {
				Nodes.emplace(Value<std::string>(node, {}));
			}
		}
		Enable = !Listen.empty();
	}
	else {
		Enable = false;
	}
}

inet::ssl_ctx_t cconfig::ssloptions_t::Context() const {
	inet::ssl_ctx_t ctx;
	if (Enable) {
		inet::SslCreateContext(ctx, Cert, Key, true);
	}
	
	return ctx;
}

void cconfig::interface_t::load(const std::filesystem::path& path, const yaml_t& options) {
	KeepAlive = Value<std::time_t>(options["keep-alive-timeout"], 0);
	Interface = Map(options["interface"], { {"pusher",api_t::type::pusher},{"push1st",api_t::type::push1st} }, api_t::type::disable);
	Path = PathValue(options["path"], "apps");
	Ssl.Cert = Value<std::string>(options["ssl"]["cert"], {});
	Ssl.Key = Value<std::string>(options["ssl"]["key"], {});
	Ssl.Enable = Value<bool>(options["ssl"]["enable"], false);

	if (options["listen"].IsDefined() and options["listen"].IsSequence()) {
		for (auto&& dsn : options["listen"]) {
			if (auto con{ dsn.as<std::string>() }; con.compare(0, 6, "tcp://") == 0) {
				Tcp = dsn;
			}
			else if (auto con{ dsn.as<std::string>() }; con.compare(0, 7, "unix://") == 0) {
				Unix = dsn;
			}
		}
	}
	else {
		Interface = api_t::type::disable;
	}
}

cconfig::credential_t::credential_t(const credential_t& cred) :
	Id{ std::move(cred.Id) }, Name{ std::move(cred.Name) }, Key{ std::move(cred.Key) }, Secret{ std::move(cred.Secret) },
	Channels{ cred.Channels }, Hooks{ std::move(cred.Hooks) }, OptionClientMessages{ cred.OptionClientMessages }, OptionStatistic{ cred.OptionStatistic },
	Origins{ std::move(cred.Origins) }, Endpoints{ std::move(cred.Endpoints) }
{
	;
}

void cconfig::credential_t::load(const std::filesystem::path& path) {
	auto&& credOptions{ yaml::load(path) };
	if (credOptions.IsDefined() and credOptions.IsMap()) {
		for (auto&& cred : credOptions) {
			Id = Value<std::string>(cred.first, {});
			Enable = Value<bool>(cred.second["enable"], Enable);
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
				OptionKeepAlive = Value<bool>(cred.second["hook"]["keep-alive"], OptionKeepAlive);
				if (cred.second["hook"]["trigger"].IsSequence()) {
					Hooks = Map(cred.second["hook"]["trigger"], {
						{"register", hook_t::type::reg},{"unregister",hook_t::type::unreg},{"join",hook_t::type::join},{"leave",hook_t::type::leave},{"push",hook_t::type::push} },
						hook_t::type::none);
					if (Hooks != hook_t::type::none) {
						if (cred.second["hook"]["endpoint"].IsSequence()) {
							for (auto&& item : cred.second["hook"]["endpoint"]) {
								if (auto endpoint{ item.as<std::string>() };
									(endpoint.compare(0, 7, "http://") == 0 or endpoint.compare(0, 8, "https://") == 0 or endpoint.compare(0, 6, "lua://") == 0)) {
									Endpoints.emplace(endpoint);
								}
							}
						}
						else if (cred.second["hook"]["endpoint"].IsScalar()) {
							if (auto endpoint{ cred.second["hook"]["endpoint"].as<std::string>() };
								(endpoint.compare(0, 7, "http://") == 0 or endpoint.compare(0, 8, "https://") == 0 or endpoint.compare(0, 6, "lua://") == 0)) {
								Endpoints.emplace(endpoint);
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

void cconfig::Load(std::filesystem::path configfile) {
	syslog.ob.print("Config file", "%s", configfile.c_str());
	try {
		std::filesystem::path progPath{ program_invocation_name };
		
		if (configfile.is_relative()) {
			configfile = progPath.parent_path() / progPath;
		}

		Path = configfile.parent_path();

		auto&& cfgOptions{ yaml::load(configfile) };

		Server.load(Path, cfgOptions["server"]);
		Cluster.load(Path, cfgOptions["cluster"]);
		Api.load(Path, cfgOptions["api"]);
		Credentials.load(Path, cfgOptions["credentials"]);
	}
	catch (std::exception& ex) {
		printf("Load config file ( %s )\n\tException ... %s\n", configfile.c_str(), ex.what());
		exit(0);
	}
}


bool cconfig::cdsn::assign(const std::string& dsn) {
	static const std::regex re(R"(^([\w-]+://?)?((?:([^:@]+)(?:\:(.+))?@)?(/?([^:/]+)(?::(\d+))?)(([^?]*)(.*))))");
	Value.assign(dsn);
	std::string_view src{ Value };
	std::cmatch match;
	if (std::regex_match(src.begin(), src.end(), match, re)) {
		Proto = std::string_view{ match[1].first, (size_t)match[1].length() };
		if (strncasecmp(Proto.data(), "http://", 7) == 0 or strncasecmp(Proto.data(), "https://", 8) == 0 or strncasecmp(Proto.data(), "tcp://", 6) == 0 or strncasecmp(Proto.data(), "udp://", 6) == 0) {
			User = std::string_view{ match[3].first,(size_t)match[3].length() };
			Pwd = std::string_view{ match[4].first,(size_t)match[4].length() };
			HostPort = std::string_view{ match[5].first,(size_t)match[5].length() };
			Host = std::string_view{ match[6].first,(size_t)match[6].length() };
			Port = std::string_view{ match[7].first,(size_t)match[7].length() };
			Url = std::string_view{ match[8].first,(size_t)match[8].length() };
			Path = std::string_view{ match[9].first,(size_t)match[9].length() };
			Args = std::string_view{ match[10].first,(size_t)match[10].length() };
		}
		else {
			Path = std::string_view{ match[2].first,(size_t)match[2].length() };
			Args = std::string_view{ match[10].first,(size_t)match[10].length() };
		}
		return true;
	}
	Value.clear();
	return false;
}