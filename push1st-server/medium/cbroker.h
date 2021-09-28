#pragma once
#include "../core/cconfig.h"
#include "../inet/cepoll.h"
#include <vector>

class chook;
class ccluster;
class ccredentials;
class cwebsocketserver;
class cchannels;

class cbroker : public std::enable_shared_from_this<cbroker>
{
public:
	cbroker();
	~cbroker();
	std::shared_ptr<chook> RegisterHook(const std::string& endpoint);

	void Initialize(const core::cconfig& config);

	int Run();

private:
	void OnIdle();
	int WaitFor(std::initializer_list<int>&& signals);
private:
	std::shared_ptr<ccluster> Cluster;
	std::shared_ptr<ccredentials> Credentials;
	std::shared_ptr<cchannels> Channels;
	std::shared_ptr<cwebsocketserver> WsServer;
	std::vector<std::shared_ptr<inet::cpoll>> ServerPoll;
	std::unordered_map<std::string, std::shared_ptr<chook>> Hooks;
};

using broker_t = std::shared_ptr<cbroker>;