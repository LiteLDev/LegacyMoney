#include "Plugin.h"
#include "ll/api/plugin/NativePlugin.h"

extern void entry(ll::plugin::NativePlugin& pl);

namespace legacymoney {

Plugin::Plugin(ll::plugin::NativePlugin& self) : mSelf(self) { entry(mSelf); }

bool Plugin::enable() { return true; }

bool Plugin::disable() { return true; }

} // namespace legacymoney