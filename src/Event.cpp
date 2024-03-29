#include "Event.h"
#include "LLMoney.h"
#include <vector>

using namespace std;

vector<LLMoneyCallback> beforeCallbacks, afterCallbacks;

bool CallBeforeEvent(LLMoneyEvent event, std::string from, std::string to, long long value) {
    bool isCancelled = false;
    for (auto& callback : beforeCallbacks) {
        if (!callback(event, from, to, value)) {
            isCancelled = true;
            break;
        }
    }
    return !isCancelled;
}

void CallAfterEvent(LLMoneyEvent event, std::string from, std::string to, long long value) {
    for (auto& callback : afterCallbacks) {
        callback(event, from, to, value);
    }
}

void LLMoney_ListenBeforeEvent(LLMoneyCallback callback) { beforeCallbacks.push_back(callback); }

void LLMoney_ListenAfterEvent(LLMoneyCallback callback) { afterCallbacks.push_back(callback); }
