#include <cstdio>
#include "medium/cbroker.h"

#ifndef TEST
#define TEST 1
#endif

int main(int argc, char* argv[])
{
#if TEST != 1
    broker_t&& broker{ std::make_shared<cbroker>() };
    try
    {
        core::cconfig cfg;
        cfg.Load("/home/irokez/projects/push1st-server/push1st-server/opt/server.yml");
        broker->Initialize(cfg);
    }
    catch (std::exception& ex) {

    }
    return 0;
#else
    extern int test_http_route(int argc, char* argv[]);
    test_http_route(argc, argv);
    return 0;
#endif
}