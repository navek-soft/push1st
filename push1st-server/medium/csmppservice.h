#pragma once
#include "../../lib/http/chttpserver.h"
#include "../../lib/inet/cunixserver.h"
#include "../../lib/http/chttpconn.h"
#include "../core/cconfig.h"
#include "ccredentials.h"
#include "cmessage.h"

class csmppservice : public std::enable_shared_from_this<csmppservice> {
	class cgateway : public std::enable_shared_from_this<cgateway> {
	public:
		cgateway(const std::string& sender, const std::string& login, const std::string& pwd, const std::vector<std::string>& hosts, const std::string& port);
		~cgateway();

		std::shared_ptr<inet::csocket> Connect();
		ssize_t Send(const inet::socket_t& so, const std::string& msg, std::string& response);

		void Assign(const std::string& sender, const std::string& login, const std::string& pwd, const std::vector<std::string>& hosts, const std::string& port);

		inline auto Seq() { return seqNo++; }
		inline auto Id() { return seqId; }

	private:
		std::string gwSender, gwLogin, gwPassword;
		std::vector<sockaddr_storage> gwHosts;
		std::shared_ptr<inet::csocket> gwSocket;
		uint32_t seqNo{ 1 }, seqId{ 4 };
		size_t gwHash{ 0 };
	};
public:
	csmppservice() = default;
	virtual ~csmppservice() = default;
	inline void Listen(const std::shared_ptr<inet::cpoll>& poll) { gwPoll.emplace_back(poll); }
	json::value_t Send(const json::value_t& message);
private:
	size_t gwIndex{ 0 };
	std::vector<std::shared_ptr<inet::cpoll>> gwPoll;
	std::unordered_map<std::string, std::shared_ptr<cgateway>> gwConnections;
};