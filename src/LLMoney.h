#pragma once

#include <vector>

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
LLMONEY_API long long LLMoney_Get(std::string xuid);
LLMONEY_API bool      LLMoney_Set(std::string xuid, long long money);
LLMONEY_API bool      LLMoney_Trans(std::string from, std::string to, long long val, std::string const& note = "");
LLMONEY_API bool      LLMoney_Add(std::string xuid, long long money);
LLMONEY_API bool      LLMoney_Reduce(std::string xuid, long long money);

LLMONEY_API std::string LLMoney_GetHist(std::string xuid, int timediff = 24 * 60 * 60);
LLMONEY_API void        LLMoney_ClearHist(int difftime = 0);

LLMONEY_API void LLMoney_ListenBeforeEvent(LLMoneyCallback callback);
LLMONEY_API void LLMoney_ListenAfterEvent(LLMoneyCallback callback);
#ifdef __cplusplus
}
#endif
LLMONEY_API std::vector<std::pair<std::string, long long>> LLMoney_Ranking(unsigned short num = 5);
