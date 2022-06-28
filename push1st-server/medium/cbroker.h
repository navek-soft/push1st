#pragma once
#include "../core/cconfig.h"
#include "../../lib/inet/cepoll.h"
#include <vector>

class chook;
class cconnection;
class ccluster;
class ccredentials;
class cwebsocketserver;
class capiserver;
class cchannels;
class cchannel;
class csmppservice;

class cbroker : public std::enable_shared_from_this<cbroker>
{
public:
	cbroker();
	~cbroker();
	std::unique_ptr<chook> RegisterHook(const std::string& endpoint, bool keepalive);

	void Initialize(const core::cconfig& config);
	std::shared_ptr<cchannel> GetChannel(const std::string& appId, const std::string& chName);
	int Run();
private:
	void OnIdle();
	int WaitFor(std::initializer_list<int>&& signals);
private:
	std::shared_ptr<ccluster> Cluster;
	std::shared_ptr<ccredentials> Credentials;
	std::shared_ptr<cchannels> Channels;
	std::shared_ptr<cwebsocketserver> WsServer;
	std::shared_ptr<capiserver> ApiServer;
	std::unordered_map<std::string, std::shared_ptr<cconnection>> HookConnections;
};

using broker_t = std::shared_ptr<cbroker>;