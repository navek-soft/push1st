#include "cbroker.h"
#include "chooks.h"
#include "ccluster.h"
#include "cchannels.h"
#include "cwebsocketserver.h"
#include "capiserver.h"
#include "ccredentials.h"
#include <csignal>
#include "../core/csyslog.h"

std::shared_ptr<chook> cbroker::RegisterHook(const std::string& endpoint) {
    if (dsn_t dsn{ endpoint }; dsn.proto() == "http://" or dsn.proto() == "https://") {
        std::string endpointId;
        endpointId.assign(dsn.proto()).append(dsn.hostport());
        if (auto&& it{ Hooks.find(endpointId) }; it != Hooks.end()) {
            return it->second;
        }
        return Hooks.emplace(endpointId, std::make_shared<cwebhook>(endpoint)).first->second;
    }
    if (auto&& it{ Hooks.find(endpoint) }; it != Hooks.end()) {
        return it->second;
    }
    return Hooks.emplace(endpoint, std::make_shared<cluahook>(endpoint)).first->second;
}

int cbroker::Run() {
    return 0;
}

void cbroker::OnIdle() {
    //printf("timeout\n");
}

void cbroker::Initialize(const core::cconfig& config) {

    if (config.Server.Proto.empty()) throw std::runtime_error("Protocols not specified");
    if (!config.Server.Threads) throw std::runtime_error("Invalid worker threads number ( zero count )");

    Channels = std::make_shared<cchannels>();
    Credentials = std::make_shared<ccredentials>(shared_from_this(), config.Credentials);
    ApiServer = std::make_shared<capiserver>(Channels, Credentials, config.Api);
    if (!config.Server.Proto.empty()) { WsServer = std::make_shared<cwebsocketserver>(Channels, Credentials, config.Server); }

    ServerPoll.reserve(config.Server.Threads);
    for (auto n{ config.Server.Threads }; n--;) {
        ServerPoll.emplace_back(std::make_shared<inet::cpoll>());
        if (WsServer) { WsServer->Listen(ServerPoll.back()); }
        ApiServer->Listen(ServerPoll.back());
        ServerPoll.back()->Listen();
    }
 
    syslog.ob.flush(1);

    WaitFor({ SIGINT, SIGQUIT, SIGABRT, SIGSEGV, SIGHUP });

    for (auto& poll : ServerPoll) {
        poll->Join();
    }
}

int cbroker::WaitFor(std::initializer_list<int>&& signals) {
    sigset_t sigSet, sigMask;
    struct sigaction sigAction;
    siginfo_t sigInfo;
    timespec tmout{ .tv_sec = 1, .tv_nsec = 0 };

    sigemptyset(&sigSet);
    sigemptyset(&sigMask);
    memset(&sigAction, 0, sizeof(sigAction));

    sigAction.sa_sigaction = [](int sig, siginfo_t* si, void* ctx) {
        psiginfo(si, "SIGNAL");
    };
    sigAction.sa_flags |= SA_SIGINFO | SA_ONESHOT;

    for (auto s : signals) {
        sigaction(s, &sigAction, nullptr);
        sigaddset(&sigSet, s);
    }

    sigprocmask(SIG_BLOCK, &sigSet, &sigMask);
    int nsig{ -1 };
    while ((nsig = sigtimedwait(&sigMask, &sigInfo, &tmout)) == -1 or errno == EAGAIN) {
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
