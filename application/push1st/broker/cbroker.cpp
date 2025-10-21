#include "cbroker.h"

#include <csignal>

#include "channels/cchannel.h"
#include "channels/cchannels.h"
#include "core/cluster/ccluster.h"
#include "core/credentials/ccredentials.h"
#include "core/hooks/chooks.h"

#include "../apiserver/capiserver.h"
#include "../websocketserver/cwebsocketserver.h"

std::shared_ptr<cchannel> cbroker::GetChannel(const std::string& appId, const std::string& chName) {
    return Channels->Get(appId, chName);
}

std::unique_ptr<chook> cbroker::RegisterHook(const std::string& endpoint, bool keepalive) {// NOLINT (static)
    if (dsn_t dsn {endpoint}; dsn.IsWeb()) {
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

int cbroker::Run() {// NOLINT (static)
    return 0;
}

void cbroker::OnIdle() {
    // static size_t stat_counter {0};
    auto List {Channels->List()};
    // if (false and syslog.Is(4) and !((++stat_counter) % 10)) {
    //     syslog.Print(4, "Channels: ( %ld )\n\t", List.size());
    //     if (!List.empty()) {
    //         syslog.Print(4, " ");
    //         size_t n {0};
    //         for (auto&& ch : List) {
    //             syslog.Print(4, "%s ( %ld ) ", ch.first.c_str(), ch.second->CountSubscribers());
    //             if (n and (n % 3) == 0) {
    //                 syslog.Print(4, "\n\t");
    //             }
    //             ++n;
    //         }
    //     }
    //     syslog.Print(4, "\n");
    // }
    for (auto&& [chName, chSelf] : List) {
        if (chSelf->Gc()) {
            continue;
        }
        Channels->UnRegister(chName);
        break;
    }
    Cluster->Ping();
    Cluster->Check();
}

void cbroker::Initialize(const core::cconfig& config) {
    if (!config.Server.Threads) {
        throw std::runtime_error("Invalid worker threads number ( zero count )");
    }

    Cluster = std::make_shared<ccluster>(shared_from_this(), config.Cluster);
    Channels = std::make_shared<cchannels>(Cluster);
    Credentials = std::make_shared<ccredentials>(shared_from_this(), config.Credentials);
    ApiServer = capiserver::MakeShared(Channels, Credentials, Cluster, config.Api);
    if (!config.Server.Proto.Empty()) {
        WsServer = std::make_shared<cwebsocketserver>(Channels, Credentials, config.Server);
    }

    std::shared_ptr<inet::cpoll> ClusterPoll {std::make_shared<inet::cpoll>("cluster-worker")};
    Cluster->Listen(ClusterPoll);
    ClusterPoll->Listen();

    WaitFor({SIGINT, SIGQUIT, SIGABRT, SIGSEGV, SIGHUP});

    ClusterPoll->Join();
    ApiServer->Join();
    WsServer->Join();
}

static volatile int nsig {-1};

int cbroker::WaitFor(std::initializer_list<int>&& signals) {
    sigset_t sigSet, sigMask;
    struct sigaction sigAction;
    siginfo_t sigInfo;
    timespec tmout {.tv_sec = 1, .tv_nsec = 0};

    sigemptyset(&sigSet);
    sigemptyset(&sigMask);
    memset(&sigAction, 0, sizeof(sigAction));

    sigAction.sa_sigaction = [](int sig, siginfo_t* si, [[maybe_unused]] void* ctx) {
        nsig = sig;
        psiginfo(si, "SIGNAL");
        /// TODO: backtrace
    };
    sigAction.sa_flags |= SA_SIGINFO | SA_ONESHOT;

    for (auto s : signals) {
        sigaction(s, &sigAction, nullptr);
        sigaddset(&sigSet, s);
    }

    // sigprocmask(SIG_BLOCK, &sigSet, &sigMask);

    int sig {-1};
    while (nsig == -1 and ((sig = sigtimedwait(&sigMask, &sigInfo, &tmout)) == -1 or errno == EAGAIN)) {// NOLINT
        OnIdle();
    }
    //    nsig = sigsuspend(&sigMask);
    // sigprocmask(SIG_UNBLOCK, &sigSet, nullptr);

    return nsig;
}

cbroker::cbroker() {}

cbroker::~cbroker() {}
