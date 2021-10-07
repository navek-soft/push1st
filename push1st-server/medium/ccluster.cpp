#include "ccluster.h"
#include "../core/csyslog.h"
#include "../core/ci/cjson.h"
#include "ccredentials.h"
#include "cbroker.h"
#include "channels/cchannel.h"

namespace proto {
	enum class op_t : uint8_t { noop = 0, ping = 1, reg = 2, unreg = 3, join = 4, leave = 5, push = 6 };
	static const size_t MaxFrameSize{ 65400 };
#pragma pack(push,1)
	struct hdr_t {
		op_t	op : 4;
		uint8_t fr : 1; /* fragmented flag */
		uint8_t	sf : 1; /* start fragment */
		uint8_t	lt : 2; /* length type : 1 (short), 2 (long), 3 (large) */
		union {
			uint8_t payload[0];
		} data;
	};
#pragma pack(pop)

	inline array_t Pack(op_t op, json::value_t&& data) {
		auto&& packedData{ json::serialize(std::move(data)) };
		array_t frame{ nullptr,packedData.size() + sizeof(hdr_t) };
		if (frame.first = std::shared_ptr<uint8_t[]>(new uint8_t[frame.second]); frame.first) {
			auto hdr{ (hdr_t*)frame.first.get() };
			hdr->fr = 0;
			hdr->sf = 1;
			hdr->lt = 0;
			hdr->op = op;
			std::memcpy(hdr->data.payload, packedData.data(), packedData.size());
			return frame;
		}
		return { nullptr,0 };
	}
	inline array_t Pack(hook_t::type trigger, json::value_t&& data) {
		switch (trigger)
		{
		case hook_t::type::push: return Pack(op_t::push, std::move(data));
		case hook_t::type::join: return Pack(op_t::join, std::move(data));
		case hook_t::type::leave: return Pack(op_t::leave, std::move(data));
		case hook_t::type::reg: return Pack(op_t::reg, std::move(data));
		case hook_t::type::unreg: return Pack(op_t::unreg, std::move(data));
		default:
			break;
		}
		return { nullptr,0 };
	}
}

inline void ccluster::OnClusterPing(struct sockaddr_storage& sa, const std::string_view& data) {
	auto ip{ inet::GetIp4(sa) };
	std::unique_lock<decltype(clusLock)> lock(clusLock);
	if (auto&& it{ clusNodes.find(ip) }; it != clusNodes.end()) {
		it->second->NodeLastActivity = std::time(nullptr);

		syslog.print(4,"[ CLUSTER ] Node %s ... PING\n", inet::GetIp(sa).c_str());
	}
	else if(std::unique_ptr<cnode> node{ new cnode }; node){
		node->NodeIp = ip; node->NodeLastActivity = std::time(nullptr);
		std::memcpy(&node->NodeAddress, &sa, sizeof(struct sockaddr_storage));
		clusNodes.emplace(ip, std::move(node));
		syslog.print(1, "[ CLUSTER ] Node %s ... ADD ( PING )\n", inet::GetIp(sa).c_str());
	}
}

inline void ccluster::OnClusterReg(struct sockaddr_storage& sa, const std::string_view& data) {
	syslog.print(4, "[ CLUSTER ] Node %s ... REG\n", inet::GetIp(sa).c_str());
}

inline void ccluster::OnClusterUnReg(struct sockaddr_storage& sa, const std::string_view& data) {
	syslog.print(4, "[ CLUSTER ] Node %s ... UNREG\n", inet::GetIp(sa).c_str());
}

inline void ccluster::OnClusterJoin(struct sockaddr_storage& sa, const std::string_view& data) {
	if (json::value_t value; json::unserialize(data, value) and value.is_object() and value.contains("app") and value.contains("channel") and value.contains("data")) {
		syslog.print(4, "[ CLUSTER ] Node %s ... JOIN\n", inet::GetIp(sa).c_str());
		if (auto&& ch{ Broker->GetChannel(value["app"].get<std::string>(),value["channel"].get<std::string>()) }; ch) {
			ch->OnClusterJoin(value["data"]);
		}
	}
	else {
		syslog.error( "[ CLUSTER ] Node %s ... JOIN ( invalid message )\n", inet::GetIp(sa).c_str());
	}
}

inline void ccluster::OnClusterLeave(struct sockaddr_storage& sa, const std::string_view& data) {
	if (json::value_t value; json::unserialize(data, value) and value.is_object() and value.contains("app") and value.contains("channel") and value.contains("data")) {
		syslog.print(4, "[ CLUSTER ] Node %s ... LEAVE\n", inet::GetIp(sa).c_str());
		if (auto&& ch{ Broker->GetChannel(value["app"].get<std::string>(),value["channel"].get<std::string>()) }; ch) {
			ch->OnClusterLeave(value["data"]);
		}
	}
	else {
		syslog.error("[ CLUSTER ] Node %s ... LEAVE ( invalid message )\n", inet::GetIp(sa).c_str());
	}
}

inline void ccluster::OnClusterPush(struct sockaddr_storage& sa, const std::string_view& data) {
	if (json::value_t value; json::unserialize(data, value) and value.is_object() and value.contains("app") and value.contains("channel") and value.contains("data")) {
		syslog.print(4, "[ CLUSTER ] Node %s ... PUSH\n", inet::GetIp(sa).c_str());
		if (auto&& ch{ Broker->GetChannel(value["app"].get<std::string>(),value["channel"].get<std::string>()) }; ch) {
			ch->OnClusterPush(message_t{ new json::value_t(value["data"]) });
		}
	}
	else {
		syslog.error("[ CLUSTER ] Node %s ... PUSH ( invalid message )\n", inet::GetIp(sa).c_str());
	}
}

inline void ccluster::Send(data_t data) {
	if (data.first and data.second <= proto::MaxFrameSize) {
		std::unique_lock<decltype(clusLock)> lock(clusLock);
		size_t nwrite{ 0 };
		ssize_t res{ 0 };
		for (auto&& [ip, node] : clusNodes) {
			if (res = clusFd.SocketSend(node->NodeAddress, data.first.get(), data.second, nwrite, 0); res == 0) {
				continue;
			}
			syslog.error("[ CLUSTER ] Send to `%s` error ( %s )\n", inet::GetIp(node->NodeAddress).c_str(), std::strerror(-(int)res));
		}
	}
	else {
		syslog.error("[ CLUSTER ] Send error ( %s )\n", "Message to long");
	}
}

void ccluster::OnUdpData(fd_t fd, const inet::ssl_t& ssl, const std::weak_ptr<inet::cpoll>& poll) {
	if (inet::csocket so{ fd,ssl }; so) {
		struct sockaddr_storage sa;
		size_t nread{ 0 };
		proto::hdr_t* frame{ (proto::hdr_t*)alloca(proto::MaxFrameSize) };
		
		if (auto res = so.SocketRecv(sa, frame, proto::MaxFrameSize, nread, MSG_DONTWAIT); res == 0) {
			switch (frame->op) {
			case proto::op_t::ping:
				OnClusterPing(sa, { (char*)frame->data.payload,nread - sizeof(proto::hdr_t) });
				break;
			case proto::op_t::reg:
				OnClusterReg(sa, { (char*)frame->data.payload,nread - sizeof(proto::hdr_t) });
				break;
			case proto::op_t::unreg:
				OnClusterUnReg(sa, { (char*)frame->data.payload,nread - sizeof(proto::hdr_t) });
				break;
			case proto::op_t::join:
				OnClusterJoin(sa, { (char*)frame->data.payload,nread - sizeof(proto::hdr_t) });
				break;
			case proto::op_t::leave:
				OnClusterLeave(sa, { (char*)frame->data.payload,nread - sizeof(proto::hdr_t) });
				break;
			case proto::op_t::push:
				OnClusterPush(sa, { (char*)frame->data.payload,nread - sizeof(proto::hdr_t) });
				break;
			}
		}
		else {
			syslog.error("[ CLUSTER ] Network error ( %s )\n", std::strerror(-(int)res));
		}
	}
	else {
		syslog.error("[ CLUSTER ] Network error ( %s )\n", std::strerror(EBADFD));
	}
}

void ccluster::Push(channel_t::type type, const app_t& app, sid_t channel, message_t&& msg)
{ 
	if (clusFd and app->IsAllowTrigger(type, hook_t::type::push)) {
		Send(proto::Pack(hook_t::type::push, { {"app", app->Id },{"channel", channel },{"data", msg::ref(msg)} }));
	}
}

void ccluster::Trigger(channel_t::type type, hook_t::type trigger, const app_t& app, sid_t channel, sid_t session, json::value_t&& data)
{ 
	if (clusFd and app->IsAllowTrigger(type, trigger)) {
		Send(proto::Pack(trigger, { {"app", app->Id },{"channel", channel },{"data",data} }));
	}
}

void ccluster::Ping() {
	if (clusFd and clusPingInterval) {
		if (auto now = std::time(nullptr); now >= clusPingTime) {
			clusPingTime = now + clusPingInterval;
			Send(proto::Pack(proto::op_t::ping, json::object_t{}));
		}
	}
}

ccluster::ccluster(const std::shared_ptr<cbroker>& broker, const config::cluster_t& config) :
	inet::cudpserver("clus:srv"), Broker{ broker }, clusPingInterval{ config.PingInterval }
{
	if (config.Enable) {
		
		syslog.ob.print("Cluster", " ... enable, listen on %s", std::string{ config.Listen.hostport() }.c_str());

		for (auto&& nIt : config.Nodes) {
			if (std::unique_ptr<cnode> node{ new cnode }; node and inet::GetSockAddr(node->NodeAddress, nIt, config.Listen.port(), AF_INET) == 0) {
				node->NodeIp = inet::GetIp4(node->NodeAddress);
				syslog.ob.print("Node", "%s ... enable, address %s", nIt.c_str(), inet::GetIp(node->NodeAddress).c_str());
				clusNodes.emplace(node->NodeIp, std::move(node));
			}
			else {
				syslog.ob.print("Node", "%s ... disabled ( address not resolved )", nIt.c_str());
			}
		}

		if (auto res = UdpListen(config.Listen.hostport(), true, true, true); res == 0) {
			clusFd = std::move(Fd());
		}
		else {
			syslog.error("Cluster initialize server error ( %s )\n", std::strerror(-(int)res));
			throw std::runtime_error("Cluster server exception");
		}
	}
	else {
		syslog.ob.print("Cluster", " ... disable");
	}
}

ccluster::~ccluster() {
	
}
