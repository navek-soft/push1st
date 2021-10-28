#pragma once
#include "medium.h"
#include "../../lib/inet/cudpserver.h"
#include "../core/ci/cjson.h"
#include "../core/ci/cspinlock.h"
#include "ccredentials.h"
#include "cmessage.h"
#include "../core/lua/clua.h"

class cbroker;

class ccluster : public inet::cudpserver, public std::enable_shared_from_this<ccluster>
{
	class cnode {
	public:
		uint32_t NodeIp{ 0 };
		std::time_t NodeLastActivity{ 0 };
		struct sockaddr_storage NodeAddress;
	};
public:
	ccluster(const std::shared_ptr<cbroker>&, const config::cluster_t&);
	virtual ~ccluster();
	void Trigger(channel_t::type type, hook_t::type trigger, const app_t& app, sid_t channel, sid_t session, json::value_t&&);
	void Push(channel_t::type type, const app_t& app, sid_t channel, message_t&& msg);
	void Ping();
protected:
	virtual void OnUdpData(fd_t fd, const inet::ssl_t& ssl, const std::weak_ptr<inet::cpoll>& poll);
	virtual inline std::shared_ptr<inet::cudpserver> UdpSelf() { return std::dynamic_pointer_cast<inet::cudpserver>(shared_from_this()); }
private:
	inline void OnClusterPing(struct sockaddr_storage& sa,const std::string_view& data);
	inline void OnClusterReg(struct sockaddr_storage& sa, const std::string_view& data);
	inline void OnClusterUnReg(struct sockaddr_storage& sa, const std::string_view& data);
	inline void OnClusterJoin(struct sockaddr_storage& sa, const std::string_view& data);
	inline void OnClusterLeave(struct sockaddr_storage& sa, const std::string_view& data);
	inline void OnClusterPush(struct sockaddr_storage& sa, const std::string_view& data);
	inline void Send(data_t data);
private:
	std::shared_ptr<cbroker> Broker;
	std::time_t clusPingInterval{ 30 }, clusPingTime{ 0 };
	sync_t clusSync{ sync_t::type::push };
	inet::csocket clusFd{ -1,nullptr };
	//inet::csocket cliFd{ -1,nullptr };
	core::cspinlock clusLock;
	std::unordered_map<uint32_t /* ip */, std::unique_ptr<cnode>> clusNodes;
	bool clusModuleAllowed{ false };
	std::filesystem::path clusModule;
	clua::engine jitLua;

	inline void CallModule(const std::string& method, std::vector<std::any>&& args);
};