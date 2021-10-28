#include "cbroker.h"
#include "chooks.h"
#include "ccluster.h"
#include "cchannels.h"
#include "channels/cchannel.h"
#include "cwebsocketserver.h"
#include "capiserver.h"
#include "ccredentials.h"
#include <csignal>
#include "../core/csyslog.h"

std::shared_ptr<cchannel> cbroker::GetChannel(const std::string& appId, const std::string& chName) {
    return Channels->Get(appId, chName);
}

std::unique_ptr<chook> cbroker::RegisterHook(const std::string& endpoint, bool keepalive) {
    if (dsn_t dsn{ endpoint }; dsn.isweb()) {
        /*std::string endpointId;
        endpointId.assign(dsn.proto()).append(dsn.hostport());
        std::shared_ptr<cconnection> connection;
        if (auto&& it{ HookConnections.find(endpointId) }; it != HookConnections.end()) {
            connection = it->second;
        }
        else {
            connection = std::make_shared<cconnection>(dsn);
            HookConnections.emplace(endpointId, connection);
        }
        */
        return std::make_unique<cwebhook>(endpoint, keepalive);
    }
    return std::make_unique<cluahook>(endpoint);
}

int cbroker::Run() {
    return 0;
}

void cbroker::OnIdle() {
    static size_t stat_counter{ 0 };
    if (syslog.is(4) and !((++stat_counter) % 10)) {
        syslog.print(4, "Channels: ( %ld )", Channels->Channels.size());
        if (!Channels->Channels.empty()) {
            syslog.print(4, " ");
            for (auto&& ch : Channels->Channels) {
                syslog.print(4, "%s ( %ld ) ", ch.first.c_str(), ch.second->CountSubscribers());
            }
        }
        syslog.print(4, "\n");
    }
    for (auto&& [chName, chSelf] : Channels->Channels) {
        if (chSelf->Gc()) {
            continue;
        }
        Channels->UnRegister(chName);
        break;
    }
    Cluster->Ping();
}

void cbroker::Initialize(const core::cconfig& config) {

    if (config.Server.Proto.empty()) throw std::runtime_error("Protocols not specified");
    if (!config.Server.Threads) throw std::runtime_error("Invalid worker threads number ( zero count )");

    Cluster = std::make_shared<ccluster>(shared_from_this(), config.Cluster);
    Channels = std::make_shared<cchannels>(Cluster);
    Credentials = std::make_shared<ccredentials>(shared_from_this(), config.Credentials);
    ApiServer = std::make_shared<capiserver>(Channels, Credentials, Cluster, config.Api);
    if (!config.Server.Proto.empty()) { WsServer = std::make_shared<cwebsocketserver>(Channels, Credentials, config.Server); }

    syslog.ob.flush(1);

    ServerPoll.reserve(config.Server.Threads);
    for (auto n{ config.Server.Threads }; n--;) {
        ServerPoll.emplace_back(std::make_shared<inet::cpoll>());
        if (WsServer) { WsServer->Listen(ServerPoll.back()); }
        ApiServer->Listen(ServerPoll.back());
        Cluster->Listen(ServerPoll.back());
        ServerPoll.back()->Listen();
    }
 

    WaitFor({ SIGINT, SIGQUIT, SIGABRT, SIGSEGV, SIGHUP });

    for (auto& poll : ServerPoll) {
        poll->Join();
    }
}

static volatile int nsig{ -1 };

int cbroker::WaitFor(std::initializer_list<int>&& signals) {
    sigset_t sigSet, sigMask;
    struct sigaction sigAction;
    siginfo_t sigInfo;
    timespec tmout{ .tv_sec = 1, .tv_nsec = 0 };

    sigemptyset(&sigSet);
    sigemptyset(&sigMask);
    memset(&sigAction, 0, sizeof(sigAction));

    sigAction.sa_sigaction = [](int sig, siginfo_t* si, void* ctx) {
        nsig = sig;
        psiginfo(si, "SIGNAL");
    };
    sigAction.sa_flags |= SA_SIGINFO | SA_ONESHOT;

    for (auto s : signals) {
        sigaction(s, &sigAction, nullptr);
        sigaddset(&sigSet, s);
    }

    sigprocmask(SIG_BLOCK, &sigSet, &sigMask);
    
    int sig{ -1 };
    while (nsig == -1 and ((sig = sigtimedwait(&sigMask, &sigInfo, &tmout)) == -1 or errno == EAGAIN)) {
        OnIdle();
    }
//    nsig = sigsuspend(&sigMask);
    sigprocmask(SIG_UNBLOCK, &sigSet, nullptr);

    return nsig;
}

cbroker::cbroker() {
}

cbroker::~cbroker() {

}
