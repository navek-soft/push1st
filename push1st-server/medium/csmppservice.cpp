#include "csmppservice.h"
#include <unistd.h>
#include <cinttypes>
#include "../core/csyslog.h"
#include <netinet/in.h>

namespace smpp {

	enum class receipt_t : uint8_t {
		none = 0, always = 1, failure = 2, success = 3
	};

	enum class bind_t : uint32_t {
		none = 0, transmitter = 2, receiver = 1, transceiver = 0x009
	};

	std::string utf8_to_utf16(const std::string& utf8)
	{
		std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> convert;
		std::u16string utf16 = convert.from_bytes(utf8);

		for (auto& ch : utf16) {
			ch = htobe16(ch);
		}

		return { (char*)utf16.data(),utf16.size() * 2 };
	}


	template <size_t __begin, size_t...__indices, typename Tuple>
	auto tuple_slice_impl(Tuple&& tuple, std::index_sequence<__indices...>) {
		return std::make_tuple(std::get<__begin + __indices>(std::forward<Tuple>(tuple))...);
	}

	template <size_t __begin, size_t __count, typename Tuple>
	auto tuple_slice(Tuple&& tuple) {
		static_assert(__count > 0, "splicing tuple to 0-length is weird...");
		return tuple_slice_impl<__begin>(std::forward<Tuple>(tuple), std::make_index_sequence<__count>());
	}

	template <size_t __begin, size_t __count, typename Tuple>
	using tuple_slice_t = decltype(tuple_slice<__begin, __count>(Tuple{}));


	class cpacker {
	public:
		cpacker(size_t reserved) { data.reserve(reserved); }
		inline cpacker& write(uint8_t v) { data.push_back(v); offset += sizeof(v); return *this; }
		inline cpacker& write(uint32_t v) { v = htobe32(v); data.append((char*)&v, sizeof(v)); offset += sizeof(v); return *this; }
		inline cpacker& write(uint16_t v) { v = htobe16(v); data.append((char*)&v, sizeof(v)); offset += sizeof(v); return *this; }
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
			uint32_t len{ 0 };
		public:
			cmd_t() = default;
			cmd_t(uint32_t _id, uint32_t _status, uint32_t _seq) : len{ sizeof(cmd_t) }, id{ _id }, status{ _status }, sequence{ _seq } {; }
			inline void operator ()(uint32_t _id, uint32_t _status, uint32_t _seq) { id = _id; status = _status; sequence = _seq; }
			inline cpacker& pack(cpacker& package) { package.write(len).write(id).write(status).write(sequence); return package; }
			inline void length(uint32_t _len) { len = htobe32(_len); }
			inline uint32_t length() { return be32toh(len); }
			inline void unpack(std::string_view& data) { 
				len = be32toh(*(uint32_t*)data.data()); data.remove_prefix(4); 
				id = be32toh(*(uint32_t*)data.data()); data.remove_prefix(4);
				status = be32toh(*(uint32_t*)data.data()); data.remove_prefix(4);
				sequence = be32toh(*(uint32_t*)data.data()); data.remove_prefix(4);
			}
		public:
			uint32_t  id{ 0 }, status{ 0 }, sequence{ 0 };
		};
#pragma pack(pop)
		class service_t {
			std::string type;
		public:
			service_t() = default;
			inline void operator ()(const std::string& id) { type = id; }
			inline cpacker& pack(cpacker& package) { package.write(type); return package; }
			inline void unpack(std::string_view& data) {
				type.assign(data.data());
				data.remove_prefix(type.size() + 1);
			}
		};
		class address_t {
			uint8_t ton{ 0 }, npi{ 0 };
			std::string address;
		public:
			address_t() = default;
			inline address_t(uint8_t _ton, uint8_t _npi, const std::string& _addr) { ton = _ton; npi = _npi; address = _addr; }
			inline void operator ()(uint8_t _ton, uint8_t _npi, const std::string& _addr) { ton = _ton; npi = _npi; address = _addr; }
			inline cpacker& pack(cpacker& package) { package.write(ton).write(npi).write(address); return package; }
			inline void unpack(std::string_view& data) {
				ton = (uint8_t)data.front();  data.remove_prefix(1);
				npi = (uint8_t)data.front();  data.remove_prefix(1);
				address.assign(data.data());
				data.remove_prefix(address.size() + 1);
			}
		};
		class esm_class_t {
			uint8_t value{ 0 };
		public:
			esm_class_t() = default;
			inline void operator ()(uint8_t val) { value = val; }
			inline void receipt(receipt_t type) { value = (value & 0xfc) | (uint8_t)type; }
			inline cpacker& pack(cpacker& package) { package.write(value); return package; }
			inline void unpack(std::string_view& data) {
				value = (uint8_t)data.front();  data.remove_prefix(1);
			}
		};
		class protocol_t {
			uint8_t value{ 0 };
		public:
			protocol_t() = default;
			inline void operator ()(uint8_t val) { value = val; }
			inline cpacker& pack(cpacker& package) { package.write(value); return package; }
			inline void unpack(std::string_view& data) {
				value = (uint8_t)data.front();  data.remove_prefix(1);
			}
		};
		class priority_t {
			uint8_t value{ 0 };
		public:
			priority_t() = default;
			inline void operator ()(uint8_t val) { value = val; }
			inline cpacker& pack(cpacker& package) { package.write(value); return package; }
			inline void unpack(std::string_view& data) {
				value = (uint8_t)data.front();  data.remove_prefix(1);
			}
		};
		class schedule_delivery_time_t {
			std::string value;
		public:
			schedule_delivery_time_t() = default;
			inline void operator ()(const std::string& val) { value = val; }
			inline cpacker& pack(cpacker& package) { package.write(value); return package; }
			inline void unpack(std::string_view& data) {
				value.assign(data.data());
				data.remove_prefix(value.size() + 1);
			}
		};
		class validity_period_t {
			std::string value;
		public:
			validity_period_t() = default;
			inline void operator ()(const std::string& val) { value = val; }
			inline cpacker& pack(cpacker& package) { package.write(value); return package; }
			inline void unpack(std::string_view& data) {
				value.assign(data.data());
				data.remove_prefix(value.size() + 1);
			}
		};
		class registered_delivery_t {
			uint8_t value{ 0 };
		public:
			registered_delivery_t() = default;
			inline void operator ()(uint8_t val) { value = val; }
			inline void unpack(std::string_view& data) {
				value = (uint8_t)data.front();  data.remove_prefix(1);
			}
			inline cpacker& pack(cpacker& package) { package.write(value); return package; }
		};
		class replace_if_present_flag_t {
			uint8_t value{ 0 };
		public:
			replace_if_present_flag_t() = default;
			inline void operator ()(uint8_t val) { value = val; }
			inline cpacker& pack(cpacker& package) { package.write(value); return package; }
			inline void unpack(std::string_view& data) {
				value = (uint8_t)data.front();  data.remove_prefix(1);
			}
		};

		class encoding_t {
			uint8_t value{ 0 };
		public:
			enum cp { smsc = 0, latin1 = 3, ucs2 = 8, cyrillic = 6 };
			encoding_t() = default;
			inline void operator ()(uint8_t val) { value = val; }
			inline cpacker& pack(cpacker& package) { package.write(value); return package; }
			inline std::string encode(const std::string& msg) { return value != encoding_t::cp::ucs2 ? msg : utf8_to_utf16(msg); }
			inline void unpack(std::string_view& data) {
				value = (uint8_t)data.front();  data.remove_prefix(1);
			}
		};

		class udh_t {
		public:
			uint8_t len{ 5 };
			uint16_t tag{ 0x3 };
			uint8_t id{ 1 }, parts{ 0 }, no{ 0 };
			inline void operator ()(uint8_t nparts, uint8_t nno) { parts = nparts; no = nno; }
			inline cpacker& pack(cpacker& package) { return package.write(len).write(tag).write(id).write(parts).write(no); }
		};

		class sm_default_msg_id_t {
			uint8_t value{ 0 };
		public:
			sm_default_msg_id_t() = default;
			inline void operator ()(uint8_t val) { value = val; }
			inline cpacker& pack(cpacker& package) { package.write(value); return package; }
			inline void unpack(std::string_view& data) {
				value = (uint8_t)data.front();  data.remove_prefix(1);
			}
		};

		class message_t {
			std::string value;
		public:
			message_t() = default;
			inline void operator ()(const std::string& msg) { value = msg; }
			inline cpacker& pack(cpacker& package) { package.write((uint8_t)value.length()).write(value.data(), value.length()); return package; }
			inline void unpack(std::string_view& data) {
				size_t len = (uint8_t)data.front(); data.remove_prefix(1);
				value.assign(data.data(), len); data.remove_prefix(len);
			}
			inline std::vector<std::string> split(const std::string& msg) {
				if (msg.length() <= 140) { /* no split 140 symbols */
					return { msg };
				}
				std::vector<std::string> parts;
				size_t nparts{ (msg.length() + 131) / 132 };
				parts.reserve(nparts);
				std::string_view msgpart{ msg };
				param::udh_t udh;
				for (size_t npart{ 1 }; !msgpart.empty();++npart) {
					cpacker package(144);
					udh((uint8_t)nparts, (uint8_t)npart);
					auto len{ std::min(132ul, msgpart.length()) };
					udh.pack(package).write(msgpart.data(), len);
					msgpart.remove_prefix(len);
					parts.emplace_back(package.str());
				}
				return parts;
			}
			const std::string& str() { return value; }
		};

		class string_t {
			std::string value;
		public:
			string_t(const std::string& val = {}) : value{ val } { ; }
			inline void operator ()(const std::string& msg) { value = msg; }
			inline cpacker& pack(cpacker& package) { package.write(value); return package; }
			inline void unpack(std::string_view& data) { 
				value = std::move(std::string{ data.data() });
				data.remove_prefix(value.size() + 1); 
			}
			inline auto str() const { return value; }
		};

		class interface_t {
			uint8_t value{ 0 };
		public:
			interface_t(uint8_t val = 0) : value{ val } { ; }
			inline void operator ()(uint8_t ver) { value = ver; }
			inline cpacker& pack(cpacker& package) { package.write(value); return package; }
		};

	}

	class cenquire {
	public:
		static inline const uint32_t req_id{ 0x00000015 };
		static inline const uint32_t resp_id{ 0x80000015 };
		param::cmd_t command;
		cenquire(uint32_t seq, uint32_t id = resp_id, uint32_t status = 0) : command{ id,status,seq } { ; }

		std::string pack() {
			cpacker packer{ 512 };
			command.pack(packer); 
			auto&& msg{ packer.str() };
			((param::cmd_t*)msg.data())->length((uint32_t)msg.length());
			return msg;
		}
	};

	class csms {
	public:
		static inline const uint32_t req_id{ 4 };
		static inline const uint32_t resp_id{ 0x80000004 };
		csms(uint32_t seq, uint32_t id = req_id, uint32_t status = 0) : command{ id,status,seq } { ; }
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
		cbind(uint32_t seq, bind_t id = bind_t::transceiver, uint32_t status = 0) : command{ (uint32_t)id,status,seq } { ; }
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

	class cdelivery_resp {
	public:
		static inline const uint32_t id{ 0x80000005 };
		cdelivery_resp(uint32_t seq, const std::string msg, uint32_t status = 0) : command{ (uint32_t)id,status,seq }, msg_id{ msg } { ; }

		std::string pack() {
			cpacker packer{ 512 };
			command.pack(packer); msg_id.pack(packer);
			auto&& msg{ packer.str() };
			((param::cmd_t*)msg.data())->length((uint32_t)msg.length());
			return msg;
		}

	private:
		param::cmd_t command;
		param::string_t msg_id;
	};

	class cdelivery {
	public:
		static inline const uint32_t id{ 5 };
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
		param::string_t predefined;
		param::message_t message;

		inline void unpack(std::string_view& data) {
			service.unpack(data);
			src.unpack(data); 
			dst.unpack(data);
			esm.unpack(data); 
			proto.unpack(data); 
			priority.unpack(data);
			delivery_time.unpack(data); 
			valid_period.unpack(data);
			register_delivery.unpack(data); replace.unpack(data);
			encoding.unpack(data); 
			predefined.unpack(data); 
			message.unpack(data);
		}
	};

	template<typename ... ARGS>
	class cresponse {
		using response_t = std::tuple<ARGS...>;
		template<typename T>
		inline void unpack(std::string_view& data, T& val) { val.unpack(data);  }
		inline void unpack(std::string_view& data) { ; }
	public:
		inline response_t operator()(std::string_view& data) {
			auto&& response{ std::tuple_cat(std::tuple<std::string_view&>(data), response_t{}) };
			std::apply([this](std::string_view& data, auto&& ... xs) {
				((std::forward<decltype(xs)>(xs).unpack(data)),...);
				}, response);
			return tuple_slice<1, std::tuple_size<response_t>::value>(response);
			//return {};
		}
	};
}

std::pair<std::string, json::value_t> csmppservice::Send(const json::value_t& message) {
	try {
		std::shared_ptr<cgateway> con;
		if (std::string gwLogin{ message.contains("login") ? message["login"].get<std::string>() : std::string{} }; !gwLogin.empty()) {
			std::string gwPassword{ message.contains("password") ? message["password"].get<std::string>() : std::string{} };
			std::vector<std::string> gwHosts;

			if (message.contains("hosts")) {
				if (message["hosts"].is_array()) {
					for (auto&& host : message["hosts"]) {
						gwHosts.emplace_back(host.get<std::string>());
					}
				}
				else if (message["hosts"].is_string()) {
					gwHosts.emplace_back(message["hosts"].get<std::string>());
				}
			}

			std::string conId{ gwLogin + ":" + gwPassword };
			{
				std::unique_lock<decltype(gwConnectionLock)> lock(gwConnectionLock);

				if (auto&& conIt{ gwConnections.find(conId) }; conIt != gwConnections.end()) {
					con = conIt->second;
					/*
					con->Assign(gwLogin, gwPassword,
						gwHosts, message.contains("port") ? message["port"].get<std::string>() : std::string{});
						*/
					syslog.print(7, "Reuse connection: %s ( channel: %s ), %lx\n", gwLogin.c_str(), con->Channel().c_str(),this);

				}
				else {
					con = gwConnections.emplace(conId, std::make_shared<cgateway>(gwPoll, gwHook, gwLogin, gwPassword,
						gwHosts, message.contains("port") ? 
							(message["port"].is_string() ? message["port"].get<std::string>() : std::to_string(message["port"].get<size_t>())) : 
							std::string{})).first->second;

					gwPoll->EnqueueGc(con);

					syslog.print(7, "New connection: %s ( channel: %s ), %lx\n", conId.c_str(), con->Channel().c_str(),this);
				}
			}

			if (con->Connect()) {
				auto&& src{ message["message"].get<std::string>() };
				smpp::csms sms{ 0 };
				sms.src(message["source_ton"].get<uint8_t>(), message["source_npi"].get<uint8_t>(), message["source_addr"].get<std::string>());
				sms.dst(message["destination_ton"].get<uint8_t>(), message["destination_npi"].get<uint8_t>(), message["destination_addr"].get<std::string>());
				sms.encoding(smpp::param::encoding_t::ucs2);
				sms.esm.receipt(smpp::receipt_t::always);

				if (message.contains("receipt")) {
					if (std::string&& receipt{ message["receipt"].get<std::string>() }; receipt == "none") {
						sms.esm.receipt(smpp::receipt_t::none);
					}
					else if (receipt == "failure") {
						sms.esm.receipt(smpp::receipt_t::failure);
					}
					else if (receipt == "success") {
						sms.esm.receipt(smpp::receipt_t::success);
					}
				}

//				auto MsgId{ con->MsgId() };
				auto&& chunks{ sms.message.split(sms.encoding.encode(src)) };

				sms.esm(chunks.size() == 1 ? 0 : 0x40);

				std::vector<uint32_t> MsgId;

				for (auto&& part : chunks) {
					sms.command.sequence = con->Seq();
					MsgId.emplace_back(sms.command.sequence);
					sms.message(part);
					auto&& tm_start{ std::chrono::system_clock::now() };
					if (auto res{ con->Send(sms.pack()) }; res == 0) {
						syslog.print(7, "Push SMS: %ld, id: %ld, seq: %ld/%ld ( src: %s, dst: %s ) ... success (%ld msec) \n",
							sms.command.status, sms.command.id, sms.command.sequence, MsgId.size(), message["source_addr"].get<std::string>().c_str(),
							message["destination_addr"].get<std::string>().c_str(), std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - tm_start).count());
						continue;
					}
					else {
						syslog.print(7, "Push SMS: %ld, id: %ld, seq: %ld/%ld ( src: %s, dst: %s ) ... error ( %s )\n", 
							sms.command.status, sms.command.id, sms.command.sequence, MsgId.size(), 
							message["destination_addr"].get<std::string>().c_str(), std::strerror(-(int)res));
						return { "400", json::object_t{{"id", MsgId},{"error",strerror(-(int)res)},{"channel", con->Channel()}} };
					}
				}
				return { "201", json::object_t{ {"id", MsgId},{"status","pending"},{"channel", con->Channel()} } };
			}
			else {
				con->Close();
				syslog.print(7, "Connection was lost ( %s )\n", con->Channel().c_str());
			}
			return { "400", json::object_t{ {"error","invalid gateway"},{"channel", con->Channel() } } };
		}
		else {
			throw std::runtime_error("invalid `sender` field");
		}
	}
	catch (std::exception& ex) {
		return { "500", json::object_t{ {"error",ex.what()} } };
	}
	return { "400", json::object_t{ {"error","invalid message format"} } };
}

ssize_t csmppservice::cgateway::Send(const std::string& msg, std::string& response) {
	if (gwSocket) {
		ssize_t err{ 0 };
		if (size_t nbytes{ 0 }; (err = gwSocket.SocketSend(msg.data(), msg.length(), nbytes, 0)) == 0 and nbytes == msg.length()) {
			response.resize(512);
			if (err = gwSocket.SocketRecv(response.data(), sizeof(smpp::param::cmd_t), nbytes, 0); err == 0 and nbytes == sizeof(smpp::param::cmd_t)) {
				auto cmd{ (smpp::param::cmd_t*)response.data() };
				response.resize(cmd->length());
				size_t nread{ response.length() - sizeof(smpp::param::cmd_t) };
				if (err = gwSocket.SocketRecv(response.data() + sizeof(smpp::param::cmd_t), nread, nbytes, 0); err == 0 and nbytes == nread) {
					return 0;
				}
			}
		}
		gwSocket.SocketClose();
		return err;
	}
	return -EBADFD;
}

ssize_t csmppservice::cgateway::Send(const std::string& msg) {
	//std::unique_lock<decltype(gwSocketLock)> lock(gwSocketLock);
	if (gwSocket) {
		ssize_t err{ 0 };
		if (size_t nbytes{ 0 }; (err = gwSocket.SocketSend(msg.data(), msg.length(), nbytes, 0)) == 0) {
			return 0;
		}
		gwSocket.SocketClose();
		return err;
	}
	return -EBADFD;
}

inline void csmppservice::cgateway::OnDeliveryStatus(const std::string& data) {
	std::string_view response{ data };
	json::value_t status;
	if (auto&& [cmd] = smpp::cresponse<smpp::param::cmd_t>{}(response); cmd.status == 0) {
		if (cmd.id == smpp::cdelivery::id) {
			auto&& [report] = smpp::cresponse<smpp::cdelivery>{}(response);

			syslog.print(7, "Delivery SMS: status: %ld, id: %ld, seq: %ld ( %s ), channel: %s\n", cmd.status, cmd.id, cmd.sequence, report.message.str().c_str(), Channel().c_str());

			if (cmd.status == 0) {
				status = json::object_t{ {"id", cmd.sequence},{"status", "delivered"},{"channel", Channel()},{"data",report.message.str()} };
			}
			else {
				status = json::object_t{ {"id", cmd.sequence},{"status", "not_delivered"},{"channel", Channel()} };
			}

			smpp::cdelivery_resp resp{ cmd.sequence,{} };
			Send(resp.pack());
		}
		else if(smpp::csms::resp_id == cmd.id)  {
			auto&& [msg] = smpp::cresponse<smpp::param::string_t>{}(response);

			if (cmd.status == 0) {
				status = json::object_t{ {"id", cmd.sequence},{"status", "accepted"},{"channel", Channel()},{"uid",msg.str()} };
				syslog.print(7, "Accept SMS: status: %ld, id: %ld, seq: %ld, uid: %s, channel: %s\n", cmd.status, cmd.id, cmd.sequence, msg.str().c_str(), Channel().c_str());
			}
			else {
				status = json::object_t{ {"id", cmd.sequence},{"status", "not_accepted"},{"channel", Channel()} };
				syslog.print(7, "Accept SMS: status: %ld, id: %ld, seq: %ld, channel: %s\n", cmd.status, cmd.id, cmd.sequence, Channel().c_str());
			}
		}
		else if (smpp::cenquire::req_id == cmd.id) {
			smpp::cenquire resp{ cmd.sequence };
			auto res = Send(resp.pack());
			syslog.print(7, "Enquire Link ( ping ): status: %ld, id: %ld, seq: %ld, channel: %s ( %s )\n", cmd.status, cmd.id, cmd.sequence, Channel().c_str(), std::strerror(-(int)res));
			return;
		}
		else if (smpp::cenquire::resp_id == cmd.id) {
			syslog.print(7, "Enquire Link ( pong ): status: %ld, id: %ld, seq: %ld, channel: %s ( %s )\n", cmd.status, cmd.id, cmd.sequence, Channel().c_str(), std::strerror(0));
			return;
		}
		else {
			syslog.print(7, "Status SMS: %ld, id: %ld, seq: %ld\n", cmd.status, cmd.id, cmd.sequence);
			status = json::object_t{ {"id", cmd.sequence},{"status", "event"},{"code", cmd.status},{"channel", Channel()} };
		}
	}
	else {
		syslog.print(7, "Invalid reply SMS: %ld, id: %ld, seq: %ld\n", cmd.status, cmd.id, cmd.sequence);
		status = json::object_t{ {"id", cmd.sequence},{"status", "error"},{"code", cmd.status},{"channel", Channel()} };
	}

	if (gwHook) {
		gwHook->Push(status["status"].get<std::string>(), status["channel"].get<std::string>(), "POST", std::move(status), {
			{"Content-Type","application/json"},
			{"Connection", "close" } });
	}
}

void csmppservice::cgateway::OnGwReply(fd_t fd, uint events) {
	if (gwSocket) {
		if (events == EPOLLIN) {
			ssize_t err{ 0 };
			std::string response; response.resize(512);
			size_t nbytes{ 0 };
			if (err = gwSocket.SocketRecv(response.data(), sizeof(smpp::param::cmd_t), nbytes, 0); err == 0 and nbytes == sizeof(smpp::param::cmd_t)) {
				auto cmd{ (smpp::param::cmd_t*)response.data() };
				response.resize(cmd->length());
				size_t nread{ response.length() - sizeof(smpp::param::cmd_t) };
				if (err = gwSocket.SocketRecv(response.data() + sizeof(smpp::param::cmd_t), nread, nbytes, MSG_WAITALL); err == 0 and nbytes == nread) {
					OnDeliveryStatus(response);
					return;
				}
			}
			syslog.print(7, "Reply error: %s\n", std::strerror(-(int)err));
		}
		gwSocket.SocketClose();
	}
}

bool csmppservice::cgateway::IsLeaveUs(std::time_t now) {
	if (gwPingTime and gwPingTime <= now) {
		if (gwSocket and gwSocket.GetErrorNo() == 0) {
			gwPingTime = std::time(nullptr) + gwPingInterval;
			smpp::cenquire enq{ Seq(), smpp::cenquire::req_id };
			syslog.print(7, "Ping: %ld ( %ld sec ) ... %s\n", gwPingTime, gwPingInterval, Send(enq.pack()) == 0 ? "success" : "fail");
		}
	}
	return false;
}

inline bool csmppservice::cgateway::Connect() {
	std::unique_lock<decltype(gwSocketLock)> lock(gwSocketLock);
	if (gwSocket and gwSocket.GetErrorNo() == 0) {
		return true;
	}
	gwPingTime = 0;
	gwSocket.SocketClose();
	for (auto&& sa : gwHosts) {
		ssize_t res{ 0 };
		if (fd_t fd; (res = inet::TcpConnect(fd, sa, false, 10000)) == 0) {

			gwSocket = inet::csocket{ fd, sa, inet::ssl_t{}, gwPoll };

			gwSocket.SetKeepAlive(true, 3, 10, 3);
			gwSocket.SetRecvTimeout(3000);
			gwSocket.SetSendTimeout(1000);

			/* Auth on the gateway */

			smpp::cbind ath{ Seq() };
			ath.login(gwLogin);
			ath.password(gwPassword);

			std::string response;
			if (res = Send(ath.pack(),response); res == 0) {
				std::string_view resp_data{ response };
				auto&& [cmd, alpha] = smpp::cresponse<smpp::param::cmd_t, smpp::param::string_t>{}(resp_data);
				if (cmd.status == 0) {
					gwSocket.Poll()->PollAdd(gwSocket.Fd(), EPOLLIN, std::bind(&csmppservice::cgateway::OnGwReply, this, std::placeholders::_1, std::placeholders::_2));
					syslog.print(7, "Connect to %x  ... success\n", be32toh(((sockaddr_in&)sa).sin_addr.s_addr));
					/*
					* Send Enquire link
					*/
					gwPingTime = std::time(nullptr) + gwPingInterval;

					smpp::cenquire enq{ Seq(), smpp::cenquire::req_id};

					return Send(enq.pack()) == 0;
				}
				else {
					syslog.print(7, "Connect to %x Reply bind   ... error ( response status error )\n", be32toh(((sockaddr_in&)sa).sin_addr.s_addr));
				}
			}
			else {
				syslog.print(7, "Connect to %x Send bind  ... error ( %s )\n", be32toh(((sockaddr_in&)sa).sin_addr.s_addr), std::strerror(-(int)res));
			}
			gwSocket.SocketClose();
		}
		else {
			syslog.print(7, "Connect to %x  ... error ( %s )\n", be32toh(((sockaddr_in&)sa).sin_addr.s_addr), std::strerror(-(int)res));
		}
	}
	return {};
}

void csmppservice::cgateway::Assign(const std::string& login, const std::string& pwd, const std::vector<std::string>& hosts, const std::string& port) {

	size_t nhash = std::_Hash_impl::hash(login) ^
		std::_Hash_impl::hash(pwd) ^ std::_Hash_impl::hash(port);

	for (auto&& host : hosts) {
		nhash ^= std::_Hash_impl::hash(host);
	}

	if (nhash == gwHash) {
		return;
	}

	if (gwConId.empty()) {
		gwConId.clear(); gwConId.resize(512);
		auto n = snprintf(gwConId.data(), 512, "%08X%08X%016lX", (uint32_t)(nhash % std::time(nullptr)), (uint32_t)std::time(nullptr), (uint64_t)nhash);
		gwConId.resize(n);
	}

	gwHash = nhash;
	gwLogin = login; gwPassword = pwd;
	
	gwHosts.clear();

	for (auto&& host : hosts) {
		sockaddr_storage sa;
		if (inet::GetSockAddr(sa, host, port, AF_INET) == 0) {
			gwHosts.emplace_back(std::move(sa));
		}
	}

	gwSocket.SocketClose();
}

csmppservice::cgateway::cgateway(const std::shared_ptr<inet::cpoll>& poll, const std::shared_ptr<cwebhook>& hook, const std::string& login, const std::string& pwd, const std::vector<std::string>& hosts, const std::string& port) :
	gwSocket{ -1,{} },gwPoll { poll }, gwHook{ hook }
{
	Assign(login, pwd, hosts, port);
}

csmppservice::cgateway::~cgateway() {
	
}

csmppservice::csmppservice(const std::string& webhook) { 
	if (!webhook.empty()) { gwHook = std::make_shared<cwebhook>(webhook); }; 
	gwPoll = std::make_shared<inet::cpoll>(); 
	syslog.ob.print("Api", "SMPP ... enabled ( Delivery report is %s )", webhook.empty() ? "Disable" : "Enable");
}
