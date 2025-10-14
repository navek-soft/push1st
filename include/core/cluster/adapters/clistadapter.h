#pragma once

#include <string_view>

#include "cadapter.h"
#include "core/util/cfactory.h"

class clistadapter : public cadapter, public cuniquefactory<clistadapter> {
    log_as(listadapter);
    friend cuniquefactory<clistadapter>;

   protected:
    clistadapter(const std::unordered_set<std::string>& nodes, const std::string_view& port);

   public:
    ~clistadapter() override = default;

   public:
    void Start() override {};
};