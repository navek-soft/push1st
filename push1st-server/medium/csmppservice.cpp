#include "csmppservice.h"

namespace smpp {
	class cpacker {
	public:
		cpacker(size_t reserved) { data.reserve(reserved); }
		inline cpacker& write(uint8_t v) { data.push_back(v); offset += sizeof(v); return *this; }
		inline cpacker& write(uint32_t v) { v = htobe32(v); data.append((char*)&v, sizeof(v)); offset += v; return *this; }
		inline cpacker& write(const std::string& v) { data.append(v.data(), v.length()).push_back('\0'); offset += v.length() + 1; return *this; }
		inline cpacker& write(const void* v,size_t len) { data.append((char*)v, len); offset += len; return *this; }
		inline std::string str() { data.shrink_to_fit(); return data; }
	private:
		size_t offset{ 0 };
		std::string data;
	};
	namespace param {
#pragma pack(push,1)
		class cmd_t {
			uint32_t len{ 0 }, id{ 0 }, status{ 0 }, sequence{ 0 };
		public:
			cmd_t() = default;
			cmd_t(uint32_t _id, uint32_t _status, uint32_t _seq) : len{ sizeof(cmd_t) }, id{ _id }, status{ _status }, sequence{ _seq } {; }
			inline void operator ()(uint32_t _id, uint32_t _status, uint32_t _seq) { id = _id; status = _status; sequence = _seq; }
			inline cpacker& pack(cpacker& package) { package.write(len).write(id).write(status).write(sequence); return package; }
			inline void length(uint32_t _len) { len = htobe32(_len); }
			inline uint32_t length() { return be32toh(len); }
			inline void unpack(std::string_view&) { ; }
		};
#pragma pack(pop)
		class service_t {
			std::string type;
		public:
			service_t() = default;
			inline void operator ()(const std::string& id) { type = id; }
			inline cpacker& pack(cpacker& package) { package.write(type); return package; }
		};
		class address_t {
			uint8_t ton{ 0 }, npi{ 0 };
			std::string address;
		public:
			address_t() = default;
			inline address_t(uint8_t _ton, uint8_t _npi, const std::string& _addr) { ton = _ton; npi = _npi; address = _addr; }
			inline void operator ()(uint8_t _ton, uint8_t _npi, const std::string& _addr) { ton = _ton; npi = _npi; address = _addr; }
			inline cpacker& pack(cpacker& package) { package.write(ton).write(npi).write(address); return package; }
		};
		class esm_class_t {
			uint8_t value{ 0 };
		public:
			esm_class_t() = default;
			inline void operator ()(uint8_t val) { value = val; }
			inline cpacker& pack(cpacker& package) { package.write(value); return package; }
		};
		class protocol_t {
			uint8_t value{ 0 };
		public:
			protocol_t() = default;
			inline void operator ()(uint8_t val) { value = val; }
			inline cpacker& pack(cpacker& package) { package.write(value); return package; }
		};
		class priority_t {
			uint8_t value{ 0 };
		public:
			priority_t() = default;
			inline void operator ()(uint8_t val) { value = val; }
			inline cpacker& pack(cpacker& package) { package.write(value); return package; }
		};
		class schedule_delivery_time_t {
			std::string value;
		public:
			schedule_delivery_time_t() = default;
			inline void operator ()(const std::string& val) { value = val; }
			inline cpacker& pack(cpacker& package) { package.write(value); return package; }
		};
		class validity_period_t {
			std::string value;
		public:
			validity_period_t() = default;
			inline void operator ()(const std::string& val) { value = val; }
			inline cpacker& pack(cpacker& package) { package.write(value); return package; }
		};
		class registered_delivery_t {
			uint8_t value{ 0 };
		public:
			registered_delivery_t() = default;
			inline void operator ()(uint8_t val) { value = val; }
			inline cpacker& pack(cpacker& package) { package.write(value); return package; }
		};
		class replace_if_present_flag_t {
			uint8_t value{ 0 };
		public:
			replace_if_present_flag_t() = default;
			inline void operator ()(uint8_t val) { value = val; }
			inline cpacker& pack(cpacker& package) { package.write(value); return package; }
		};

		class encoding_t {
			uint8_t value{ 0 };
		public:
			encoding_t() = default;
			inline void operator ()(uint8_t val) { value = val; }
			inline cpacker& pack(cpacker& package) { package.write(value); return package; }
		};

		class sm_default_msg_id_t {
			uint8_t value{ 0 };
		public:
			sm_default_msg_id_t() = default;
			inline void operator ()(uint8_t val) { value = val; }
			inline cpacker& pack(cpacker& package) { package.write(value); return package; }
		};

		class message_t {
			std::string value;
		public:
			message_t() = default;
			inline void operator ()(const std::string& msg) { value = msg; }
			inline cpacker& pack(cpacker& package) { package.write((uint8_t)value.length()).write(value.data(), value.length()); return package; }
		};

		class string_t {
			std::string value;
		public:
			string_t(const std::string& val = {}) : value{ val } { ; }
			inline void operator ()(const std::string& msg) { value = msg; }
			inline cpacker& pack(cpacker& package) { package.write(value); return package; }
			inline void unpack(std::string_view&) { ; }
		};

		class interface_t {
			uint8_t value{ 0 };
		public:
			interface_t(uint8_t val = 0) : value{ val } { ; }
			inline void operator ()(uint8_t ver) { value = ver; }
			inline cpacker& pack(cpacker& package) { package.write(value); return package; }
		};

	}

	class csms {
	public:
		csms(uint32_t seq, uint32_t id = 4, uint32_t status = 0) : command{ id,status,seq } { ; }
		param::cmd_t command;
		param::service_t service;
		param::address_t src, dst;
		param::esm_class_t esm;
		param::protocol_t proto;
		param::priority_t priority;
		param::schedule_delivery_time_t delivery_time;
		param::validity_period_t valid_period;
		param::registered_delivery_t register_delivery;
		param::replace_if_present_flag_t replace;
		param::encoding_t encoding;
		param::sm_default_msg_id_t msg_id;
		param::message_t message;
		std::string pack() { 
			cpacker packer{ 512 }; 
			command.pack(packer); service.pack(packer);
			src.pack(packer); dst.pack(packer); 
			esm.pack(packer); proto.pack(packer);
			priority.pack(packer); delivery_time.pack(packer);
			valid_period.pack(packer); register_delivery.pack(packer);
			replace.pack(packer);  encoding.pack(packer);
			msg_id.pack(packer);  message.pack(packer);

			auto&& msg{ packer.str() };
			((param::cmd_t*)msg.data())->length((uint32_t)msg.length());
			return msg;
		}
	};

	class cbind {
	public:
		cbind(uint32_t seq,uint32_t id = 2, uint32_t status = 0) : command{ id,status,seq } { ; }
		param::cmd_t command;
		param::string_t login, password, system{};
		param::interface_t version{ 0x34 };
		param::address_t addr{ 0,0,{} };
		std::string pack() {
			cpacker packer{ 512 };
			command.pack(packer); login.pack(packer);
			password.pack(packer); system.pack(packer);
			version.pack(packer); addr.pack(packer);

			auto&& msg{ packer.str() };
			((param::cmd_t*)msg.data())->length((uint32_t)msg.length());
			return msg;
		}
	};

	class cunbind {

	};

	template<typename ... ARGS>
	class cresponse {
		using response_t = std::tuple<ARGS...>;
		template<typename T>
		inline void unpack(std::string_view& data, T& val) { val.unpack(data);  }
		inline void unpack(std::string_view& data) { ; }
	public:
		inline response_t operator()(std::string_view data) {
			response_t response;
			
			std::apply([this](std::string_view& data, auto&&... xs) {
				((std::forward<decltype(xs)>(xs).unpack(data)),...);
				//((unpack<decltype(xs)>(data,std::forward<decltype(xs)>(xs)), ...));
				//(unpack<decltype(xs)>(data, std::forward<decltype(xs)>(xs)),...);
			}, std::tuple_cat(std::tuple<std::string_view&>(data), response));
		}
	};
}

json::value_t csmppservice::Send(const json::value_t& message) {
	try {
		std::shared_ptr<cgateway> con;
		if (std::string gwSender{ message.contains("sender") ? message["sender"].get<std::string>() : std::string{} }; !gwSender.empty()) {
			std::string gwLogin{ message.contains("login") ? message["login"].get<std::string>() : std::string{} };
			std::string gwPassword{ message.contains("password") ? message["password"].get<std::string>() : std::string{} };
			std::vector<std::string> gwHosts;

			if (message.contains("hosts") and message["hosts"].is_array()) {
				for (auto&& host : message["hosts"]) {
					gwHosts.emplace_back(host.get<std::string>());
				}
			}

			if (auto&& conIt{ gwConnections.find(gwSender + ":" + gwLogin) }; conIt != gwConnections.end()) {
				con = conIt->second;
				con->Assign(gwSender, gwLogin, gwPassword,
					gwHosts, message.contains("port") ? message["port"].get<std::string>() : std::string{});

			}
			else {
				con = std::make_shared<cgateway>(gwSender, gwLogin, gwPassword,
					gwHosts, message.contains("port") ? message["port"].get<std::string>() : std::string{});
			}

			smpp::csms sms{ con->Seq() };
			sms.src(message["source_ton"].get<uint8_t>(), message["source_npi"].get<uint8_t>(), message["source_addr"].get<std::string>());
			sms.dst(message["destination_ton"].get<uint8_t>(), message["destination_npi"].get<uint8_t>(), message["destination_addr"].get<std::string>());
			sms.encoding(3);
			sms.message(message["message"].get<std::string>());

			std::string response;
			if (auto res{ con->Send(con->Connect(), sms.pack(),response) }; res == 0) {

				auto&& [cmd, sys] = smpp::cresponse<smpp::param::cmd_t, smpp::param::string_t>{}(response);

				return {};
			}

			/*
			message.contains("source_ton") ? message.get<uint8_t>() : (uint8_t)0, message.contains("source_npi") ? message.get<uint8_t>() : (uint8_t)0,
					message.contains("destination_ton") ? message.get<uint8_t>() : (uint8_t)0, message.contains("destination_npi") ? message.get<uint8_t>() : (uint8_t)0,
			*/
		}
		else {
			throw std::runtime_error("invalid `sender` field");
		}
	}
	catch (std::exception& ex) {
		printf("%s\n", ex.what());
		return json::object_t{ {"error",ex.what()} };
	}
	return {};
}

ssize_t csmppservice::cgateway::Send(const inet::socket_t& so, const std::string& msg, std::string& response) {
	if (so) {
		ssize_t err{ 0 };
		if (size_t nbytes{ 0 }; (err = so->SocketSend(msg.data(), msg.length(), nbytes, 0)) == 0 and nbytes == msg.length()) {
			response.resize(512);
			if (err = so->SocketRecv(response.data(), sizeof(smpp::param::cmd_t), nbytes, MSG_WAITALL); err == 0 and nbytes == sizeof(smpp::param::cmd_t)) {
				auto cmd{ (smpp::param::cmd_t*)response.data() };
				response.resize(cmd->length());
				size_t nread{ response.length() - sizeof(smpp::param::cmd_t) };
				if (err = so->SocketRecv(response.data() + sizeof(smpp::param::cmd_t), nread, nbytes, MSG_WAITALL); err == 0 and nbytes == nread) {
					return 0;
				}
			}
		}
		return err;
	}
	return -EBADFD;
}

std::shared_ptr<inet::csocket> csmppservice::cgateway::Connect() {
	std::shared_ptr<inet::csocket> so{ gwSocket };
	if (so and so->GetErrorNo() == 0) {
		return so;
	}
	else {
		//gwSocket.reset();
		for (auto&& sa : gwHosts) {
			if (fd_t fd; inet::TcpConnect(fd, sa, false, 10000) == 0) {
				so = std::make_shared<inet::csocket>(fd, sa, inet::ssl_t{}, std::weak_ptr<inet::cpoll>{});
				so->SetKeepAlive(true, 3, 3, 3);
				so->SetRecvTimeout(3000);
				so->SetSendTimeout(3000);

				/* Auth on the gateway */

				smpp::cbind ath{ Seq() };
				ath.login(gwLogin);
				ath.password(gwPassword);

				std::string response;
				if (auto res{ Send(so, ath.pack(),response) }; res == 0) {
					gwSocket = so;
					return so;
				}
			}
		}
	}

	return {};

}

void csmppservice::cgateway::Assign(const std::string& sender, const std::string& login, const std::string& pwd, const std::vector<std::string>& hosts, const std::string& port) {

	size_t nhash = std::_Hash_impl::hash(sender) ^ std::_Hash_impl::hash(login) ^
		std::_Hash_impl::hash(pwd) ^ std::_Hash_impl::hash(port);

	for (auto&& host : hosts) {
		nhash ^= std::_Hash_impl::hash(host);
	}

	if (nhash == gwHash) {
		return;
	}

	gwHash = nhash;
	gwSender = sender; gwLogin = login; gwPassword = pwd;
	
	gwHosts.clear();

	for (auto&& host : hosts) {
		sockaddr_storage sa;
		if (inet::GetSockAddr(sa, host, port, AF_INET) == 0) {
			gwHosts.emplace_back(std::move(sa));
		}
	}

	gwSocket.reset();
}

csmppservice::cgateway::cgateway(const std::string& sender, const std::string& login, const std::string& pwd, const std::vector<std::string>& hosts, const std::string& port) {
	Assign(sender, login, pwd, hosts, port);
}

csmppservice::cgateway::~cgateway() {

}