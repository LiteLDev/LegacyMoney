#include <string>

namespace legacy_money {
struct MoneyConfig {
    int         version         = 1;
    std::string language        = "en";
    int         def_money       = 0;
    float       pay_tax         = 0.0;
    bool        enable_commands = true;
    std::string currency_symbol = "$";
};

bool         loadConfig();
MoneyConfig& getConfig();
} // namespace legacy_money
