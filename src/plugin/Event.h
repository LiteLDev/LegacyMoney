#pragma once

#include "LLMoney.h"

bool CallBeforeEvent(LLMoneyEvent event, std::string from, std::string to, long long value);

void CallAfterEvent(LLMoneyEvent event, std::string from, std::string to, long long value);