#pragma once

#include "channels/cchannel.h"
#include "core/hooks/chooks.h"

class cibroker {
   public:
    virtual ~cibroker() {}

   public:
    virtual std::unique_ptr<chook> RegisterHook(const std::string& endpoint, bool keepalive) = 0;
    virtual std::shared_ptr<cchannel> GetChannel(const std::string& appId, const std::string& chName) = 0;
};

using broker_t = std::shared_ptr<cibroker>;