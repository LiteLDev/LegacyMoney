#include "Config.h"
#include "Event.h"
#include "LLMoney.h"
#include "LegacyMoney.h"
#include "ll/api/service/PlayerInfo.h"
#include "sqlitecpp/SQLiteCpp.h"
#include <memory>
#include <optional>
#include <string>
#include <vector>


static std::unique_ptr<SQLite::Database> db;
#undef snprintf

struct cleanSTMT {
    SQLite::Statement& get;

    cleanSTMT(SQLite::Statement& g) : get(g) {}

    ~cleanSTMT() {
        get.reset();
        get.clearBindings();
    }
};

void ConvertData();
namespace legacy_money {
bool initDatabase() {
    try {
        db = std::make_unique<SQLite::Database>(
            LegacyMoney::getInstance().getSelf().getModDir() / "economy.db",
            SQLite::OPEN_CREATE | SQLite::OPEN_READWRITE
        );
        db->exec("PRAGMA journal_mode = MEMORY");
        db->exec("PRAGMA synchronous = NORMAL");
        db->exec("CREATE TABLE IF NOT EXISTS money ( \
			XUID  TEXT PRIMARY KEY \
			UNIQUE \
			NOT NULL, \
			Money NUMERIC NOT NULL \
		) \
			WITHOUT ROWID; ");
        db->exec("CREATE TABLE IF NOT EXISTS mtrans ( \
			tFrom TEXT  NOT NULL, \
			tTo   TEXT  NOT NULL, \
			Money NUMERIC  NOT NULL, \
			Time  NUMERIC NOT NULL \
			DEFAULT(strftime('%s', 'now')), \
			Note  TEXT \
		);");
        db->exec("CREATE INDEX IF NOT EXISTS idx ON mtrans ( \
			Time COLLATE BINARY COLLATE BINARY DESC \
		); ");
    } catch (std::exception const& e) {
        legacy_money::LegacyMoney::getInstance().getSelf().getLogger().error("Database error: {}", e.what());
        return false;
    }
    ConvertData();
    return true;
}
} // namespace legacy_money

long long LLMoney_Get(std::string xuid) {
    if (xuid.empty()) {
        return -1;
    }
    try {
        SQLite::Statement get{*db, "select Money from money where XUID=?"};
        get.bindNoCopy(1, xuid);
        long long rv = legacy_money::getConfig().def_money;
        bool      fg = false;
        while (get.executeStep()) {
            rv = (long long)get.getColumn(0).getInt64();
            fg = true;
        }
        get.reset();
        get.clearBindings();
        if (!fg) {
            SQLite::Statement set{*db, "insert into money values (?,?)"};
            set.bindNoCopy(1, xuid);
            set.bind(2, legacy_money::getConfig().def_money);
            set.exec();
            set.reset();
            set.clearBindings();
        }
        return rv;
    } catch (std::exception const& e) {
        legacy_money::LegacyMoney::getInstance().getSelf().getLogger().error("Database error: {}\n", e.what());
        return -1;
    }
}

bool isRealTrans = true;

bool LLMoney_Trans(std::string from, std::string to, long long val, std::string const& note) {
    bool isRealTrans = ::isRealTrans;
    ::isRealTrans    = true;
    if (isRealTrans) {
        if (!CallBeforeEvent(LLMoneyEvent::Trans, from, to, val)) {
            return false;
        }
    }
    if (val < 0 || from == to) {
        return false;
    }
    try {
        db->exec("begin");
        SQLite::Statement set{*db, "update money set Money=? where XUID=?"};
        if (!from.empty()) {
            auto fmoney = LLMoney_Get(from);
            if (fmoney < val) {
                db->exec("rollback");
                return false;
            }
            fmoney -= val;
            {
                set.bindNoCopy(2, from);
                set.bind(1, fmoney);
                set.exec();
                set.reset();
                set.clearBindings();
            }
        }
        if (!to.empty()) {
            auto tmoney = LLMoney_Get(to);
            if (from.empty()) {
                tmoney += val;
            } else {
                tmoney += val - val * legacy_money::getConfig().pay_tax;
            }
            if (tmoney < 0) {
                db->exec("rollback");
                return false;
            }
            {
                set.bindNoCopy(2, to);
                set.bind(1, tmoney);
                set.exec();
                set.reset();
                set.clearBindings();
            }
        }

        {
            SQLite::Statement addTrans{*db, "insert into mtrans (tFrom,tTo,Money,Note) values (?,?,?,?)"};
            addTrans.bindNoCopy(1, from);
            addTrans.bindNoCopy(2, to);
            addTrans.bind(3, val);
            addTrans.bindNoCopy(4, note);
            addTrans.exec();
            addTrans.reset();
            addTrans.clearBindings();
        }
        db->exec("commit");

        if (isRealTrans) {
            CallAfterEvent(LLMoneyEvent::Trans, from, to, val);
        }
        return true;
    } catch (std::exception const& e) {
        db->exec("rollback");
        legacy_money::LegacyMoney::getInstance().getSelf().getLogger().error("Database error: {}\n", e.what());
        return false;
    }
}

bool LLMoney_Add(std::string xuid, long long money) {
    if (xuid.empty() || !CallBeforeEvent(LLMoneyEvent::Add, {}, xuid, money)) {
        return false;
    }

    isRealTrans = false;
    bool res    = LLMoney_Trans({}, xuid, money, "add " + std::to_string(money));
    if (res) CallAfterEvent(LLMoneyEvent::Add, {}, xuid, money);
    return res;
}

bool LLMoney_Reduce(std::string xuid, long long money) {
    if (xuid.empty() || !CallBeforeEvent(LLMoneyEvent::Reduce, {}, xuid, money)) {
        return false;
    }

    isRealTrans = false;
    bool res    = LLMoney_Trans(xuid, {}, money, "reduce " + std::to_string(money));
    if (res) CallAfterEvent(LLMoneyEvent::Reduce, {}, xuid, money);
    return res;
}

bool LLMoney_Set(std::string xuid, long long money) {
    if (xuid.empty() || !CallBeforeEvent(LLMoneyEvent::Set, {}, xuid, money)) {
        return false;
    }
    long long   now = LLMoney_Get(xuid), diff;
    std::string from, to;
    if (money >= now) {
        to   = xuid;
        diff = money - now;
    } else {
        from = xuid;
        diff = now - money;
    }

    isRealTrans = false;
    bool res    = LLMoney_Trans(from, to, diff, "set to " + std::to_string(money));
    if (res) CallAfterEvent(LLMoneyEvent::Set, {}, xuid, money);
    return res;
}

std::vector<std::pair<std::string, long long>> LLMoney_Ranking(unsigned short num) {
    try {
        SQLite::Statement                              get{*db, "select * from money ORDER BY money DESC LIMIT ?"};
        std::vector<std::pair<std::string, long long>> mapTemp;
        get.bind(1, num);
        while (get.executeStep()) {
            std::string xuid = get.getColumn(0).getString();
            if (xuid.empty()) {
                continue;
            }
            long long balance = get.getColumn(1).getInt64();
            // std::cout << xuid << " " << balance << "\n";
            mapTemp.push_back(std::pair<std::string, long long>(xuid, balance));
        }
        get.reset();
        get.clearBindings();
        return mapTemp;
    } catch (std::exception const& e) {
        legacy_money::LegacyMoney::getInstance().getSelf().getLogger().error("Database error: {}\n", e.what());
        return {};
    }
}

std::string LLMoney_GetHist(std::string xuid, int timediff) {
    if (xuid.empty()) {
        return {};
    }
    try {
        SQLite::Statement get{
            *db,
            "select tFrom,tTo,Money,datetime(Time,'unixepoch', 'localtime'),Note from mtrans where "
            "strftime('%s','now')-time<? and (tFrom=? OR tTo=?) ORDER BY Time DESC"
        };
        std::string rv;
        get.bind(1, timediff);
        get.bindNoCopy(2, xuid);
        get.bindNoCopy(3, xuid);
        while (get.executeStep()) {
            std::string              fromXuid = get.getColumn(0).getString();
            std::string              toXuid   = get.getColumn(1).getString();
            std::string              fromName, toName = "System";
            ll::service::PlayerInfo& info      = ll::service::PlayerInfo::getInstance();
            auto                     fromEntry = fromXuid.empty() ? std::nullopt : info.fromXuid(fromXuid);
            auto                     toEntry   = toXuid.empty() ? std::nullopt : info.fromXuid(toXuid);
            if (fromEntry) {
                fromName = fromEntry->name;
            }
            if (toEntry) {
                toName = toEntry->name;
            }
            rv += fromName + " -> " + toName + " " + std::to_string((long long)get.getColumn(2).getInt64()) + " "
                + get.getColumn(3).getText() + " (" + get.getColumn(4).getText() + ")\n";
        }
        get.reset();
        get.clearBindings();
        return rv;
    } catch (std::exception const& e) {
        legacy_money::LegacyMoney::getInstance().getSelf().getLogger().error("Database error: {}\n", e.what());
        return {};
    }
}

void LLMoney_ClearHist(int difftime) {
    try {
        db->exec("DELETE FROM mtrans WHERE strftime('%s','now')-time>" + std::to_string(difftime));
    } catch (std::exception&) {}
}

void ConvertData() {
    if (std::filesystem::exists(
            legacy_money::LegacyMoney::getInstance().getSelf().getModDir() / "LLMoney" / "money.db"
        )) {
        legacy_money::LegacyMoney::getInstance().getSelf().getLogger().info(
            "Old money data detected, try to convert old data to new data"
        );
        try {
            std::unique_ptr<SQLite::Database> db2 = std::make_unique<SQLite::Database>(
                legacy_money::LegacyMoney::getInstance().getSelf().getModDir() / "LLMoney" / "money.db",
                SQLite::OPEN_CREATE | SQLite::OPEN_READWRITE
            );
            SQLite::Statement get{*db2, "select hex(XUID),Money from money"};
            SQLite::Statement set{*db, "insert into money values (?,?)"};
            while (get.executeStep()) {
                std::string        blob = get.getColumn(0).getText();
                unsigned long long value;
                std::istringstream iss(blob);
                iss >> std::hex >> value;
                unsigned long long xuid  = _byteswap_uint64(value);
                long long          money = get.getColumn(1).getInt64();
                auto xuidStr = std::to_string(xuid);
                set.bindNoCopy(1, xuidStr);
                set.bind(2, money);
                set.exec();
                set.reset();
                set.clearBindings();
            }
            get.reset();
        } catch (std::exception& e) {
            legacy_money::LegacyMoney::getInstance().getSelf().getLogger().error("{}", e.what());
        }
        std::filesystem::rename(
            legacy_money::LegacyMoney::getInstance().getSelf().getModDir() / "LLMoney" / "money.db",
            legacy_money::LegacyMoney::getInstance().getSelf().getModDir() / "LLMoney" / "money_old.db"
        );
        legacy_money::LegacyMoney::getInstance().getSelf().getLogger().info("Conversion completed");
    }
}
