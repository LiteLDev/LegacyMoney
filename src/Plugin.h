#pragma once

#include <ll/api/plugin/NativePlugin.h>


namespace legacymoney {

[[nodiscard]] auto getSelfPluginInstance() -> ll::plugin::NativePlugin&;

} // namespace legacymoney
