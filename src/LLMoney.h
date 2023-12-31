#pragma once

#include "ll/api/service/PlayerInfo.h"

#ifdef LLMONEY_EXPORTS
#define LLMONEY_API __declspec(dllexport)
#else
#define LLMONEY_API __declspec(dllimport)
#endif

#include <string>

enum LLMoneyEvent { Set, Add, Reduce, Trans };

typedef bool (*LLMoneyCallback)(LLMoneyEvent type, std::string from, std::string to, long long value);

#ifdef __cplusplus
extern "C" {
#endif
LLMONEY_API long long LLMoneyGet(std::string xuid);
LLMONEY_API bool      LLMoneySet(std::string xuid, long long money);
LLMONEY_API bool      LLMoneyTrans(std::string from, std::string to, long long val, std::string const& note = "");
LLMONEY_API bool      LLMoneyAdd(std::string xuid, long long money);
LLMONEY_API bool      LLMoneyReduce(std::string xuid, long long money);

LLMONEY_API std::string LLMoneyGetHist(std::string xuid, int timediff = 24 * 60 * 60);
LLMONEY_API void        LLMoneyClearHist(int difftime = 0);

LLMONEY_API void LLMoneyListenBeforeEvent(LLMoneyCallback callback);
LLMONEY_API void LLMoneyListenAfterEvent(LLMoneyCallback callback);
#ifdef __cplusplus
}
#endif
LLMONEY_API std::vector<std::pair<std::string, long long>> LLMoneyRanking(unsigned short num = 5);
// Old interface
// Just for compatibility
// Do not use
namespace Money {
LLMONEY_API long long getMoney(std::string xuid);
LLMONEY_API std::string getTransHist(std::string xuid, int timediff = 24 * 60 * 60);
LLMONEY_API bool        createTrans(std::string from, std::string to, long long val, std::string const& note = "");
LLMONEY_API bool        setMoney(std::string xuid, long long money);
LLMONEY_API bool        reduceMoney(std::string xuid, long long money);
LLMONEY_API void        purgeHist(int difftime = 0);
} // namespace Money