#include <cstdio>
#include "medium/cbroker.h"

int main()
{
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
}