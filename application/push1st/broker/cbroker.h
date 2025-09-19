#pragma once
#include <core/config/cconfig.h>
#include <inet/cepoll.h>
#include <log/clog.h>

#include "broker/api.h"

class chook;
class cconnection;
class ccluster;
class ccredentials;
class cwebsocketserver;
class capiserver;
class cchannels;
class cchannel;
class csmppservice;

class cbroker : public cibroker, public std::enable_shared_from_this<cbroker> {
    log_as(broker);

   public:
    cbroker();
    ~cbroker() override;
    std::unique_ptr<chook> RegisterHook(const std::string& endpoint, bool keepalive) override;

    void Initialize(const core::cconfig& config);
    std::shared_ptr<cchannel> GetChannel(const std::string& appId, const std::string& chName) override;
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
