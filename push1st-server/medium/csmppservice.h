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
		cgateway(const std::shared_ptr<inet::cpoll>& poll, const std::string& sender, const std::string& login, const std::string& pwd, const std::vector<std::string>& hosts, const std::string& port);
		~cgateway();

		std::shared_ptr<inet::csocket> Connect();
		ssize_t Send(const inet::socket_t& so, const std::string& msg, std::string& response);

		void Assign(const std::string& sender, const std::string& login, const std::string& pwd, const std::vector<std::string>& hosts, const std::string& port);

		inline auto Seq() { return seqNo++; }
	private:
		void OnGwReply(fd_t, uint);
	private:
		std::string gwSender, gwLogin, gwPassword;
		std::vector<sockaddr_storage> gwHosts;
		std::shared_ptr<inet::csocket> gwSocket;
		uint32_t seqNo{ 0 };
		size_t gwHash{ 0 };
		std::shared_ptr<inet::cpoll> gwPoll;
	};
public:
	csmppservice() { gwPoll = std::make_shared<inet::cpoll>(); }
	virtual ~csmppservice() { gwPoll->Join(); };
	inline void Listen() { gwPoll->Listen(); }
	json::value_t Send(const json::value_t& message);
private:
	std::unordered_map<std::string, std::shared_ptr<cgateway>> gwConnections;
	std::shared_ptr<inet::cpoll> gwPoll;
};