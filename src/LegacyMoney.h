#pragma once

#include "ll/api/mod/NativeMod.h"

namespace legacy_money {

class LegacyMoney {

public:
    static LegacyMoney& getInstance();

    LegacyMoney() : mSelf(*ll::mod::NativeMod::current()) {}

    [[nodiscard]] ll::mod::NativeMod& getSelf() const { return mSelf; }

    bool load();

    bool enable();

    bool disable();

private:
    ll::mod::NativeMod& mSelf;
};

} // namespace legacy_money
