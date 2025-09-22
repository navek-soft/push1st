#include <core/util/ccmd.h>
#include <log/clog.h>
#include <version/version.h>

#include <cstdio>
#include <cstdlib>

#include "broker/cbroker.h"

int main(int argc, char* argv[]) {
    std::srand((uint)std::time(nullptr));
    /*
    auto srvSmpp{ std::make_shared<csmppservice>() };
    srvSmpp->Listen();
    auto res = srvSmpp->Send(json::object_t{
        {"source_ton",5},
        {"source_npi",0},
        {"source_addr","BTKTest"},
        {"destination_ton",1},
        {"destination_npi",1},
        {"destination_addr","375447476539"},
        {"hosts",json::array_t{"82.209.225.100","82.209.225.102"}},
        {"port","2775"},
        {"login","MY.BELTEL.NAT"},
        {"password","Qq4sw7Vy"},
        {"message","###ssh root@testmyapi.beltelecom.by -p 20022ssh root@testmyapi.beltelecom.by -p 20022ssh root@testmyapi.beltelecom.by -p 20022ssh root@testmyapi.beltelecom.by -p 20022ssh root@testmyapi.beltelecom.by -p 20022ssh root@testmyapi.beltelecom.by -p 20022$$$"}
    });

    printf("%s\n", json::serialize(std::move(res)).c_str());

    std::this_thread::sleep_for(std::chrono::seconds(40));

    return 0;
    */

    core::ccmdline cmd;
    cmd.Option("config", 'c', 1, "Location of the config yaml file", nullptr, "server.yaml")
        .Option("help", 'h', 0, "Show command line usage parameters")
        .Option("version", 'v', 0, "Show version and exit")
        .Option("log", 'l', 1, "Log file store path");

    cmd(argc, argv);

    if (cmd.Isset("log")) {
        std::filesystem::path logPath {cmd["log"]};
        auto f = std::filesystem::path {logPath / "push1st.log"};
        if (stdout = freopen(std::filesystem::path {logPath / "push1st.log"}.c_str(), "ate", stdout); !stdout) {
            perror("Log file open error: ");
            exit(1);
        }
        if (stderr = freopen(std::filesystem::path {logPath / "push1st.err"}.c_str(), "ate", stderr); !stderr) {
            perror("Log file open error: ");
            exit(1);
        }
    }

    if (cmd.Isset("version")) {
        printf("Version: %s, build: %s-%s\n", Version.Major, Version.Revision, Version.BuildType);
        return 0;
    }

    if (cmd.Isset("help")) {
        cmd.Print();
        return 0;
    }

    std::filesystem::path configFile {cmd.Isset("config") ? cmd["config"] : "./server.yaml"};

#ifndef TEST
    auto&& broker {std::make_shared<cbroker>()};
    try {
        core::cconfig cfg;
        cfg.Load(configFile);
        util::log::initialize(cfg.Logger);
        broker->Initialize(cfg);
    } catch (std::exception& ex) {
        fmt::print("Startup error: {}\n", ex.what());
        return EXIT_FAILURE;
    }
    return 0;
#else
    extern int test_http_route(int argc, char* argv[]);
    test_http_route(argc, argv);
    return 0;
#endif
}