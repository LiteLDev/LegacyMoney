#include "Plugin.h"
#include "ll/api/plugin/NativePlugin.h"

extern void entry(ll::plugin::NativePlugin& pl);

namespace legacymoney {

Plugin::Plugin(ll::plugin::NativePlugin& self) : mSelf(self) {
    // Code for loading the plugin goes here.
    entry(mSelf);
}

bool Plugin::enable() {
    // Code for enabling the plugin goes here.
    mSelf.getLogger().warn("LegacyMoney enable");
    return true;
}

bool Plugin::disable() {
    // Code for disabling the plugin goes here.
    mSelf.getLogger().warn("LegactyMoney disable");
    return true;
}

} // namespace legacymoney