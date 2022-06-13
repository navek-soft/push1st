#include <cstdio>
#include "medium/cbroker.h"
#include "core/ci/ccmd.h"
#include "core/csyslog.h"
#include "medium/csmppservice.h"
#include "version.h"

#ifndef TEST
#define TEST 0
#endif

int main(int argc, char* argv[])
{
    std::srand((uint)std::time(nullptr));

    auto srvSmpp{ std::make_shared<csmppservice>() };

    srvSmpp->Send(json::object_t{ 
        {"sender","BTKTest"},
        {"source_ton",5},
        {"source_npi",0},
        {"source_addr","555"},
        {"destination_ton",1},
        {"destination_npi",1},
        {"destination_addr","375447476539"},
        {"hosts",json::array_t{"82.209.225.100","82.209.225.102"}},
        {"port","2775"},
        {"login","MY.BELTEL.NAT"},
        {"password","Qq4sw7Vy"},
        {"message","Hello Wikipages"}
    });

    return 0;

    core::ccmdline cmd; cmd.
        option("config", 'c', 1, "Location of the config yaml file", nullptr, "server.yaml").
        option("help", 'h', 0, "Show command line usage parameters").
        option("version", 'v', 0, "Show version and exit").
        option("log", 'l', 1, "Log file store path").
        option("verbose", 'V', 1, "Set logging verbosity level", nullptr, "Value between 0 - 7", "4")(argc, argv);

    if (cmd.isset("log")) {
        std::filesystem::path logPath{ cmd["log"] };
        auto f = std::filesystem::path{ logPath / "push1st.log" };
        if (stdout = freopen(std::filesystem::path{ logPath / "push1st.log" }.c_str(), "ate", stdout); !stdout) {
            perror("Log file open error: ");
            exit(1);
        }
        if (stderr = freopen(std::filesystem::path{ logPath / "push1st.err" }.c_str(), "ate", stderr); !stderr) {
            perror("Log file open error: ");
            exit(1);
        }
    }

    if (cmd.isset("version")) { printf("Version: %s, build: %s-%s\n", version::number, version::revision, version::build); return 0; }
    if (cmd.isset("help")) { cmd.print(); return 0; }
    if (cmd.isset("verbose")) { syslog.open(std::stoul(std::string{ cmd["verbose"] })); }
    else { syslog.open(std::stoul(std::string{ 1 })); }

    std::filesystem::path configFile{ cmd.isset("config") ? cmd["config"] : "./server.yaml" };

#if TEST != 1
    broker_t&& broker{ std::make_shared<cbroker>() };
    try
    {
        core::cconfig cfg;
        cfg.Load(configFile);
        broker->Initialize(cfg);
    }
    catch (std::exception& ex) {
        syslog.bt(30);
    }
    return 0;
#else
    extern int test_http_route(int argc, char* argv[]);
    test_http_route(argc, argv);
    return 0;
#endif
}