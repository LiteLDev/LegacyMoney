#include "Nlohmann/json.hpp"

namespace Settings {
extern std::string language;
extern int         def_money;
extern float       pay_tax;
extern bool        enable_ranking;
extern bool        enable_commands;

nlohmann::json globaljson();

void initjson(nlohmann::json json);

void WriteDefaultConfig(const std::string& fileName);

void LoadConfigFromJson(const std::string& fileName);

void reloadJson(const std::string& fileName);
} // namespace Settings
