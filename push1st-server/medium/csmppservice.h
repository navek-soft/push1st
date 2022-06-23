#pragma once
#include "../../lib/http/chttpserver.h"
#include "../../lib/inet/cunixserver.h"
#include "../../lib/http/chttpconn.h"
#include "../core/cconfig.h"
#include "ccredentials.h"
#include "cmessage.h"
#include <atomic>
#include <mutex>
#include "chooks.h"

/*
* https://github.com/onlinecity/cpp-smpp/
*/

class csmppservice : public std::enable_shared_from_this<csmppservice> {
	class cgateway : public std::enable_shared_from_this<cgateway> {
	public:
		cgateway(const std::shared_ptr<inet::cpoll>& poll, const std::shared_ptr<cwebhook>& hook, const std::string& login, const std::string& pwd, const std::vector<std::string>& hosts, const std::string& port);
		~cgateway();

		inline bool Connect();
		ssize_t Send(const std::string& msg, std::string& response);
		ssize_t Send(const std::string& msg);
		void Assign(const std::string& login, const std::string& pwd, const std::vector<std::string>& hosts, const std::string& port);

		inline uint32_t Seq() { return ++seqNo; }
		inline uint32_t MsgId() { return seqNo; }
		inline const std::string& Channel() { return gwConId; }
		inline int Fd() const { return gwSocket.Fd(); }
	private:
		void OnGwReply(fd_t, uint);
		inline void OnDeliveryStatus(const std::string& response);
	private:
		std::string gwLogin, gwPassword;
		std::vector<sockaddr_storage> gwHosts;
		mutable std::mutex gwSocketLock;
//		mutable std::shared_ptr<inet::csocket> gwSocket;
		inet::csocket gwSocket;
		mutable std::atomic_uint32_t seqNo{ 0 };
		size_t gwHash{ 0 };
		std::string gwConId;
		std::shared_ptr<inet::cpoll> gwPoll;
		std::shared_ptr<cwebhook> gwHook;
	};
public:
	csmppservice(const std::string& webhook = {});
	virtual ~csmppservice() { gwPoll->Join(); };
	inline void Listen() { gwPoll->Listen(); }
	std::pair<std::string, json::value_t> Send(const json::value_t& message);
private:
	std::mutex gwConnectionLock;
	std::unordered_map<std::string, std::shared_ptr<cgateway>> gwConnections;
	std::shared_ptr<inet::cpoll> gwPoll;
	std::shared_ptr<cwebhook> gwHook;
};