#include "Plugin.h"
#include "LLMoney.h"
#include "Plugin.h"
#include "Settings.h"
#include "ll/api/Logger.h"
#include "ll/api/command/CommandHandle.h"
#include "ll/api/command/CommandRegistrar.h"
#include "ll/api/i18n/I18n.h"
#include "ll/api/plugin/NativePlugin.h"
#include "ll/api/service/Bedrock.h"
#include "ll/api/service/PlayerInfo.h"
#include "mc/common/wrapper/optional_ref.h"
#include "mc/server/commands/CommandOriginType.h"
#include "mc/server/commands/CommandOutput.h"
#include "mc/server/commands/CommandPermissionLevel.h"
#include "mc/server/commands/CommandSelector.h"
#include "mc/world/actor/player/Player.h"
#include "mc/world/level/Level.h"
#include "nlohmann/json.hpp"
#include "nlohmann/json_fwd.hpp"
#include <string>

#define JSON1(key, val)                                                                                                \
    if (json.find(key) != json.end()) {                                                                                \
        const nlohmann::json& out = json.at(key);                                                                      \
        out.get_to(val);                                                                                               \
    }

namespace Settings {
std::string language        = "en";
int         def_money       = 0;
float       pay_tax         = 0.0;
bool        enable_commands = true;
std::string currency_symbol = "$";

nlohmann::json globaljson() {
    nlohmann::json json;
    json["language"]        = language;
    json["def_money"]       = def_money;
    json["pay_tax"]         = pay_tax;
    json["enable_commands"] = enable_commands;
    json["currency_symbol"] = currency_symbol;
    return json;
}

void initjson(nlohmann::json json) {
    JSON1("language", language);
    JSON1("def_money", def_money);
    JSON1("pay_tax", pay_tax);
    JSON1("enable_commands", enable_commands);
    JSON1("currency_symbol", currency_symbol);
}
void WriteDefaultConfig(const std::string& fileName) {

    std::ofstream file(fileName);
    if (!file.is_open()) {
        legacymoney::getSelfPluginInstance().getLogger().error("Can't open file {}", fileName);
        return;
    }
    auto json = globaljson();
    file << json.dump(4);
    file.close();
}

void LoadConfigFromJson(const std::string& fileName) {
    std::ifstream file(fileName);
    if (!file.is_open()) {
        legacymoney::getSelfPluginInstance().getLogger().error("Can't open file {}", fileName);
        return;
    }
    nlohmann::json json;
    file >> json;
    file.close();
    initjson(json);
    WriteDefaultConfig(fileName);
}
void reloadJson(const std::string& fileName) {
    std::ofstream file(fileName);
    if (file) {
        file << globaljson().dump(4);
    } else {
        legacymoney::getSelfPluginInstance().getLogger().error("Configuration File Creation failed!");
    }
    file.close();
}
} // namespace Settings

bool initDB();

bool cmp(std::pair<std::string, long long> a, std::pair<std::string, long long> b) { return a.second > b.second; }


using ll::i18n_literals::operator""_tr;

void loadCfg() {
    // config
    if (std::filesystem::exists("plugins/LegacyMoney/money.json")) {
        try {
            Settings::LoadConfigFromJson("plugins/LegacyMoney/money.json");
        } catch (std::exception& e) {
            legacymoney::getSelfPluginInstance().getLogger().error(
                "Configuration file is Invalid, Error: {}",
                e.what()
            );
        } catch (...) {
            legacymoney::getSelfPluginInstance().getLogger().error("Configuration file is Invalid");
        }
    } else {
        Settings::WriteDefaultConfig("plugins/LegacyMoney/money.json");
    }
}

struct QueryMoney {
    std::string playerName;
};

struct QueryMoneySelector {
    CommandSelector<Player> player;
};

enum MoneyOperation : int { add = 0, reduce = 1, set = 2, pay = 3 };

struct OperateMoney {
    MoneyOperation operation;
    std::string    playerName;
    int            amount;
};

enum MoneyOperationSelector : int { adds = 0, reduces = 1, sets = 2 };

struct OperateMoneySelector {
    MoneyOperationSelector  operation;
    CommandSelector<Player> player;
    int                     amount;
};

struct MoneyOthers {
    std::string playerName;
    int         time = 0;
};

struct TopMoney {
    int number;
};

void RegisterMoneyCommands() {
    using ll::command::CommandRegistrar;
    auto& command =
        ll::command::CommandRegistrar::getInstance().getOrCreateCommand("money", "LegacyMoney's main command"_tr());
    command.overload<QueryMoney>()
        .text("query")
        .optional("playerName")
        .execute<[&](CommandOrigin const& origin, CommandOutput& output, QueryMoney const& param, Command const&) {
            if (param.playerName.empty()) {
                if (origin.getOriginType() != CommandOriginType::Player) {
                    output.error("Please type the name of player you want to query"_tr());
                } else {
                    Actor* actor = origin.getEntity();
                    if (actor) {
                        output.success("Your balance: "_tr()
                                           .append(Settings::currency_symbol)
                                           .append(std::to_string(LLMoney_Get(static_cast<Player*>(actor)->getXuid())))
                        );
                    } else {
                        output.error("Player not found"_tr());
                    }
                }
            } else {
                if (origin.getPermissionsLevel() >= CommandPermissionLevel::GameDirectors) {
                    auto info = ll::service::PlayerInfo::getInstance().fromName(param.playerName);
                    if (info.has_value()) {
                        output.success(
                            param.playerName
                            + "'s balance: "_tr()
                                  .append(Settings::currency_symbol)
                                  .append(std::to_string(LLMoney_Get(info->xuid)))
                        );

                    } else {
                        output.error("Player not found"_tr());
                    }
                } else {
                    output.error("You don't have permission to do this"_tr());
                }
            }
        }>();
    command.overload<QueryMoneySelector>()
        .text("querys")
        .required("player")
        .execute<
            [&](CommandOrigin const& origin, CommandOutput& output, QueryMoneySelector const& param, Command const&) {
                if (origin.getPermissionsLevel() >= CommandPermissionLevel::GameDirectors) {
                    if (param.player.results(origin).size()) {
                        auto it = param.player.results(origin).data;
                        for (Player* player : *it) {
                            if (player) {
                                output.success(
                                    player->getRealName()
                                    + "'s balance: "_tr()
                                          .append(Settings::currency_symbol)
                                          .append(std::to_string(LLMoney_Get(player->getXuid())))
                                );
                            }
                        }
                    } else {
                        output.error("Player not found"_tr());
                    }
                } else {
                    output.error("You don't have permission to do this"_tr());
                }
            }>();
    command.overload<OperateMoney>()
        .required("operation")
        .required("playerName")
        .required("amount")
        .execute<[&](CommandOrigin const& origin, CommandOutput& output, OperateMoney const& param, Command const&) {
            switch (param.operation) {
            case MoneyOperation::add: {
                if (origin.getPermissionsLevel() >= CommandPermissionLevel::GameDirectors) {
                    auto info = ll::service::PlayerInfo::getInstance().fromName(param.playerName);
                    if (info.has_value()) {
                        if (LLMoney_Add(info->xuid, param.amount)) {
                            output.success(
                                "Added "_tr() + Settings::currency_symbol + std::to_string(param.amount) + " to "_tr()
                                + param.playerName
                            );
                        } else {
                            output.error("Failed to add money"_tr());
                        }
                    } else {
                        output.error("Player not found"_tr());
                    }
                } else {
                    output.error("You don't have permission to do this"_tr());
                }
                break;
            }
            case MoneyOperation::reduce: {
                if (origin.getPermissionsLevel() >= CommandPermissionLevel::GameDirectors) {
                    auto info = ll::service::PlayerInfo::getInstance().fromName(param.playerName);
                    if (info.has_value()) {
                        if (LLMoney_Reduce(info->xuid, param.amount)) {
                            output.success(
                                "Reduced "_tr() + Settings::currency_symbol + std::to_string(param.amount) + " to "_tr()
                                + param.playerName
                            );
                        } else {
                            output.error("Failed to reduce money"_tr());
                        }
                    } else {
                        output.error("Player not found"_tr());
                    }
                } else {
                    output.error("You don't have permission to do this"_tr());
                }
                break;
            }
            case MoneyOperation::set: {
                if (origin.getPermissionsLevel() >= CommandPermissionLevel::GameDirectors) {
                    auto info = ll::service::PlayerInfo::getInstance().fromName(param.playerName);
                    if (info.has_value()) {
                        if (LLMoney_Set(info->xuid, param.amount)) {
                            output.success(
                                "Set "_tr() + param.playerName + "'s money to "_tr() + Settings::currency_symbol
                                + std::to_string(param.amount)
                            );
                        } else {
                            output.error("Failed to set money"_tr());
                        }
                    } else {
                        output.error("Player not found"_tr());
                    }
                } else {
                    output.error("You don't have permission to do this"_tr());
                }
                break;
            }
            case MoneyOperation::pay: {
                if (origin.getOriginType() == CommandOriginType::Player) {
                    auto info = ll::service::PlayerInfo::getInstance().fromName(param.playerName);
                    if (info.has_value()) {
                        Actor* fromActor = origin.getEntity();
                        if (fromActor) {
                            LLMoney_Trans(static_cast<Player*>(fromActor)->getXuid(), info->xuid, param.amount);
                        } else {
                            output.error("Origin not found!"_tr());
                        }
                    } else {
                        output.error("Player not found"_tr());
                    }
                    break;
                } else {
                    output.error("Console is not allowed to use this command"_tr());
                }
            }
            default:
                break;
            }
        }>();
    command.overload<OperateMoneySelector>()
        .required("operation")
        .required("player")
        .required("amount")
        .execute<
            [&](CommandOrigin const& origin, CommandOutput& output, OperateMoneySelector const& param, Command const&) {
                switch (param.operation) {
                case MoneyOperationSelector::adds: {
                    if (origin.getPermissionsLevel() >= CommandPermissionLevel::GameDirectors) {
                        if (param.player.results(origin).size()) {
                            auto it = param.player.results(origin).data;
                            for (Player* player : *it) {
                                if (LLMoney_Add(player->getXuid(), param.amount)) {
                                    output.success(
                                        "Added "_tr() + Settings::currency_symbol + std::to_string(param.amount)
                                        + " to "_tr() + player->getRealName()
                                    );
                                } else {
                                    output.error("Failed to reduce money"_tr());
                                }
                            }
                        } else {
                            output.error("Player not found"_tr());
                        }
                    } else {
                        output.error("You don't have permission to do this"_tr());
                    }
                    break;
                }
                case MoneyOperationSelector::reduces: {
                    if (origin.getPermissionsLevel() >= CommandPermissionLevel::GameDirectors) {
                        if (param.player.results(origin).size()) {
                            auto it = param.player.results(origin).data;
                            for (Player* player : *it) {
                                if (LLMoney_Reduce(player->getXuid(), param.amount)) {
                                    output.success(
                                        "Reduced "_tr() + Settings::currency_symbol + std::to_string(param.amount)
                                        + " to "_tr() + player->getRealName()
                                    );
                                } else {
                                    output.error("Failed to reduce money"_tr());
                                }
                            }
                        } else {
                            output.error("Player not found"_tr());
                        }
                    } else {
                        output.error("You don't have permission to do this"_tr());
                    }
                    break;
                }
                case MoneyOperationSelector::sets: {
                    if (origin.getPermissionsLevel() >= CommandPermissionLevel::GameDirectors) {
                        if (param.player.results(origin).size()) {
                            auto it = param.player.results(origin).data;
                            for (Player* player : *it) {
                                if (LLMoney_Set(player->getXuid(), param.amount)) {
                                    output.success(
                                        "Set "_tr() + Settings::currency_symbol + std::to_string(param.amount)
                                        + " to "_tr() + player->getRealName()
                                    );
                                } else {
                                    output.error("Failed to set money"_tr());
                                }
                            }
                        } else {
                            output.error("Player not found"_tr());
                        }
                    } else {
                        output.error("You don't have permission to do this"_tr());
                    }
                    break;
                }
                default:
                    break;
                }
            }>();
    command.overload<MoneyOthers>()
        .text("hist")
        .required("playerName")
        .optional("time")
        .execute<[&](CommandOrigin const& origin, CommandOutput& output, MoneyOthers const& param, Command const&) {
            if (origin.getPermissionsLevel() >= CommandPermissionLevel::GameDirectors) {
                auto info = ll::service::PlayerInfo::getInstance().fromName(param.playerName);
                if (info.has_value()) {
                    if (param.time) {
                        output.success(LLMoney_GetHist(info->xuid, param.time));
                    } else {
                        output.success(LLMoney_GetHist(info->xuid));
                    }
                } else {
                    output.error("Player not found"_tr());
                }
            } else {
                output.error("You don't have permission to do this"_tr());
            }
        }>();
    command.overload<MoneyOthers>()
        .text("hist")
        .optional("time")
        .execute<[&](CommandOrigin const& origin, CommandOutput& output, MoneyOthers const& param, Command const&) {
            if (origin.getOriginType() == CommandOriginType::Player) {
                Actor* actor = origin.getEntity();
                if (actor) {
                    if (param.time) {
                        output.success(LLMoney_GetHist(static_cast<Player*>(actor)->getXuid(), param.time));
                    } else {
                        output.success(LLMoney_GetHist(static_cast<Player*>(actor)->getXuid()));
                    }
                } else {
                    output.error("Player not found"_tr());
                }
            } else {
                output.error("Console is not allowed to use this command"_tr());
            }
        }>();
    command.overload<MoneyOthers>()
        .text("purge")
        .optional("time")
        .execute<[&](CommandOrigin const& origin, CommandOutput& output, MoneyOthers const& param, Command const&) {
            if (origin.getPermissionsLevel() >= CommandPermissionLevel::GameDirectors) {
                output.error(
                    "It's a dangerous operation which will clean all the economy history database, if you confrm "
                    "that, please type /money purge confirm "_tr()
                    + std::to_string(param.time)
                );
            } else {
                output.error("You don't have permission to do this"_tr());
            }
        }>();
    command.overload<MoneyOthers>()
        .text("purge")
        .text("confirm")
        .optional("time")
        .execute<[&](CommandOrigin const& origin, CommandOutput& output, MoneyOthers const& param, Command const&) {
            if (origin.getPermissionsLevel() >= CommandPermissionLevel::GameDirectors) {
                LLMoney_ClearHist(param.time);
                output.success("Clear history successfully"_tr());
            } else {
                output.error("You don't have permission to do this"_tr());
            }
        }>();
    command.overload<TopMoney>()
        .text("top")
        .optional("number")
        .execute<[&](CommandOrigin const& origin, CommandOutput& output, TopMoney const& param, Command const&) {
            if (param.number) {
                auto rank = LLMoney_Ranking(
                    (param.number > 100 && origin.getPermissionsLevel() == CommandPermissionLevel::Any) ? 100
                                                                                                        : param.number
                );
                output.success("Money ranking:"_tr());
                for (auto i : rank) {
                    auto info = ll::service::PlayerInfo::getInstance().fromXuid(i.first);
                    if (info.has_value()) {
                        output.success("{} {}{}", info->name, Settings::currency_symbol, i.second);
                    }
                }
            } else {
                auto rank = LLMoney_Ranking(10);
                output.success("Money ranking:"_tr());
                for (auto i : rank) {
                    auto info = ll::service::PlayerInfo::getInstance().fromXuid(i.first);
                    if (info.has_value()) {
                        output.success("{} {}{}", info->name, Settings::currency_symbol, i.second);
                    }
                }
            }
        }>();
}

namespace legacymoney {

namespace {

std::unique_ptr<std::reference_wrapper<ll::plugin::NativePlugin>>
    selfPluginInstance; // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

auto disable(ll::plugin::NativePlugin& /*self*/) -> bool { return true; }

auto enable(ll::plugin::NativePlugin& /*self*/) -> bool {
    if (Settings::enable_commands) {
        RegisterMoneyCommands();
    }
    return true;
    return true;
}

auto load(ll::plugin::NativePlugin& self) -> bool {
    auto& logger       = self.getLogger();
    selfPluginInstance = std::make_unique<std::reference_wrapper<ll::plugin::NativePlugin>>(self);
    logger.info("Loaded!");
    loadCfg();
    if (!initDB()) {
        return false;
    }
    ll::i18n::getInstance() = std::make_unique<ll::i18n::MultiFileI18N>(
        ll::i18n::MultiFileI18N("plugins/LegacyMoney/lang", Settings::language)
    );
    return true;
}

auto unload(ll::plugin::NativePlugin& self) -> bool { return true; }

} // namespace

auto getSelfPluginInstance() -> ll::plugin::NativePlugin& {
    if (!selfPluginInstance) {
        throw std::runtime_error("selfPluginInstance is null");
    }

    return *selfPluginInstance;
}

} // namespace legacymoney

extern "C" {
_declspec(dllexport) auto ll_plugin_disable(ll::plugin::NativePlugin& self) -> bool {
    return legacymoney::disable(self);
}
_declspec(dllexport) auto ll_plugin_enable(ll::plugin::NativePlugin& self) -> bool { return legacymoney::enable(self); }
_declspec(dllexport) auto ll_plugin_load(ll::plugin::NativePlugin& self) -> bool { return legacymoney::load(self); }
_declspec(dllexport) auto ll_plugin_unload(ll::plugin::NativePlugin& self) -> bool { return legacymoney::unload(self); }
}
