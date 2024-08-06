#include "LegacyMoney.h"
#include "Config.h"
#include "LLMoney.h"
#include "ll/api/Config.h"
#include "ll/api/Logger.h"
#include "ll/api/command/CommandHandle.h"
#include "ll/api/command/CommandRegistrar.h"
#include "ll/api/i18n/I18n.h"
#include "ll/api/mod/NativeMod.h"
#include "ll/api/mod/RegisterHelper.h"
#include "ll/api/service/PlayerInfo.h"
#include "ll/api/utils/ErrorUtils.h"
#include "mc/common/wrapper/optional_ref.h"
#include "mc/server/commands/CommandOriginType.h"
#include "mc/server/commands/CommandOutput.h"
#include "mc/server/commands/CommandPermissionLevel.h"
#include "mc/server/commands/CommandSelector.h"
#include "mc/world/actor/player/Player.h"
#include "mc/world/level/Level.h"
#include <string>

using namespace ll::i18n_literals;

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
        .execute([&](CommandOrigin const& origin, CommandOutput& output, QueryMoney const& param, Command const&) {
            if (param.playerName.empty()) {
                if (origin.getOriginType() != CommandOriginType::Player) {
                    output.error("Please type the name of player you want to query"_tr());
                } else {
                    Actor* actor = origin.getEntity();
                    if (actor) {
                        output.success("Your balance: "_tr()
                                           .append(legacy_money::getConfig().currency_symbol)
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
                                  .append(legacy_money::getConfig().currency_symbol)
                                  .append(std::to_string(LLMoney_Get(info->xuid)))
                        );

                    } else {
                        output.error("Player not found"_tr());
                    }
                } else {
                    output.error("You don't have permission to do this"_tr());
                }
            }
        });
    command.overload<QueryMoneySelector>().text("querys").required("player").execute(
        [&](CommandOrigin const& origin, CommandOutput& output, QueryMoneySelector const& param, Command const&) {
            if (origin.getPermissionsLevel() >= CommandPermissionLevel::GameDirectors) {
                if (param.player.results(origin).size()) {
                    auto it = param.player.results(origin).data;
                    for (Player* player : *it) {
                        if (player) {
                            output.success(
                                player->getRealName()
                                + "'s balance: "_tr()
                                      .append(legacy_money::getConfig().currency_symbol)
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
        }
    );
    command.overload<OperateMoney>()
        .required("operation")
        .required("playerName")
        .required("amount")
        .execute([&](CommandOrigin const& origin, CommandOutput& output, OperateMoney const& param, Command const&) {
            switch (param.operation) {
            case MoneyOperation::add: {
                if (origin.getPermissionsLevel() >= CommandPermissionLevel::GameDirectors) {
                    auto info = ll::service::PlayerInfo::getInstance().fromName(param.playerName);
                    if (info.has_value()) {
                        if (LLMoney_Add(info->xuid, param.amount)) {
                            output.success(
                                "Added "_tr() + legacy_money::getConfig().currency_symbol + std::to_string(param.amount)
                                + " to "_tr() + param.playerName
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
                                "Reduced "_tr() + legacy_money::getConfig().currency_symbol
                                + std::to_string(param.amount) + " to "_tr() + param.playerName
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
                                "Set "_tr() + param.playerName + "'s money to "_tr()
                                + legacy_money::getConfig().currency_symbol + std::to_string(param.amount)
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
        });
    command.overload<OperateMoneySelector>()
        .required("operation")
        .required("player")
        .required("amount")
        .execute(
            [&](CommandOrigin const& origin, CommandOutput& output, OperateMoneySelector const& param, Command const&) {
                switch (param.operation) {
                case MoneyOperationSelector::adds: {
                    if (origin.getPermissionsLevel() >= CommandPermissionLevel::GameDirectors) {
                        if (param.player.results(origin).size()) {
                            auto it = param.player.results(origin).data;
                            for (Player* player : *it) {
                                if (LLMoney_Add(player->getXuid(), param.amount)) {
                                    output.success(
                                        "Added "_tr() + legacy_money::getConfig().currency_symbol
                                        + std::to_string(param.amount) + " to "_tr() + player->getRealName()
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
                                        "Reduced "_tr() + legacy_money::getConfig().currency_symbol
                                        + std::to_string(param.amount) + " to "_tr() + player->getRealName()
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
                                        "Set "_tr() + legacy_money::getConfig().currency_symbol
                                        + std::to_string(param.amount) + " to "_tr() + player->getRealName()
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
            }
        );
    command.overload<MoneyOthers>()
        .text("hist")
        .required("playerName")
        .optional("time")
        .execute([&](CommandOrigin const& origin, CommandOutput& output, MoneyOthers const& param, Command const&) {
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
        });
    command.overload<MoneyOthers>().text("hist").optional("time").execute(
        [&](CommandOrigin const& origin, CommandOutput& output, MoneyOthers const& param, Command const&) {
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
        }
    );
    command.overload<MoneyOthers>().text("purge").optional("time").execute(
        [&](CommandOrigin const& origin, CommandOutput& output, MoneyOthers const& param, Command const&) {
            if (origin.getPermissionsLevel() >= CommandPermissionLevel::GameDirectors) {
                output.error(
                    "It's a dangerous operation which will clean all the economy history database, if you confrm "
                    "that, please type /money purge confirm "_tr()
                    + std::to_string(param.time)
                );
            } else {
                output.error("You don't have permission to do this"_tr());
            }
        }
    );
    command.overload<MoneyOthers>().text("purge").text("confirm").optional("time").execute(
        [&](CommandOrigin const& origin, CommandOutput& output, MoneyOthers const& param, Command const&) {
            if (origin.getPermissionsLevel() >= CommandPermissionLevel::GameDirectors) {
                LLMoney_ClearHist(param.time);
                output.success("Clear history successfully"_tr());
            } else {
                output.error("You don't have permission to do this"_tr());
            }
        }
    );
    command.overload<TopMoney>().text("top").optional("number").execute(
        [&](CommandOrigin const& origin, CommandOutput& output, TopMoney const& param, Command const&) {
            if (param.number) {
                auto rank = LLMoney_Ranking(
                    (param.number > 100 && origin.getPermissionsLevel() == CommandPermissionLevel::Any) ? 100
                                                                                                        : param.number
                );
                output.success("Money ranking:"_tr());
                for (auto i : rank) {
                    auto info = ll::service::PlayerInfo::getInstance().fromXuid(i.first);
                    if (info.has_value()) {
                        output.success("{} {}{}", info->name, legacy_money::getConfig().currency_symbol, i.second);
                    }
                }
            } else {
                auto rank = LLMoney_Ranking(10);
                output.success("Money ranking:"_tr());
                for (auto i : rank) {
                    auto info = ll::service::PlayerInfo::getInstance().fromXuid(i.first);
                    if (info.has_value()) {
                        output.success("{} {}{}", info->name, legacy_money::getConfig().currency_symbol, i.second);
                    }
                }
            }
        }
    );
}

namespace legacy_money {

static std::unique_ptr<LegacyMoney> instance;
LegacyMoney&                        LegacyMoney::getInstance() { return *instance; }
MoneyConfig                         config;

bool loadConfig() {
    try {
        if (ll::config::loadConfig(
                config,
                legacy_money::LegacyMoney::getInstance().getSelf().getModDir() / "money.json"
            )) {
            return true;
        }
    } catch (...) {
        legacy_money::LegacyMoney::getInstance().getSelf().getLogger().error("Failed to load configuration");
        ll::error_utils::printCurrentException(legacy_money::LegacyMoney::getInstance().getSelf().getLogger());
    }
    try {
        if (ll::config::saveConfig(
                config,
                legacy_money::LegacyMoney::getInstance().getSelf().getModDir() / "money.json"
            )) {
            return true;
        } else {
            legacy_money::LegacyMoney::getInstance().getSelf().getLogger().error("Failed to rewrite configuration");
        }
    } catch (...) {
        legacy_money::LegacyMoney::getInstance().getSelf().getLogger().error("Failed to rewrite configuration");
        ll::error_utils::printCurrentException(legacy_money::LegacyMoney::getInstance().getSelf().getLogger());
    }
    return false;
}

MoneyConfig& getConfig() { return config; }

bool initDatabase();

bool LegacyMoney::load() {
    if (!loadConfig() || !initDatabase()) {
        return false;
    }
    ll::i18n::getInstance() = std::make_unique<ll::i18n::MultiFileI18N>(
        ll::i18n::MultiFileI18N("plugins/LegacyMoney/lang", legacy_money::getConfig().language)
    );
    return true;
}

bool LegacyMoney::enable() {
    if (legacy_money::getConfig().enable_commands) {
        RegisterMoneyCommands();
    }
    return true;
}

bool LegacyMoney::disable() { return true; }

LL_REGISTER_MOD(LegacyMoney, instance);

} // namespace legacy_money
