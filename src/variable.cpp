#include "variable.h"

Variable::Variable(){}

Variable::Variable(const std::string name, const std::string type) : name_(name), type_(type) {}

void Variable::AddUpdateValueCallback(std::function<void()> callback) {
    update_value_callbacks.push_back(callback);
}

void Variable::SetValue(int value, uint64_t timestamp, bool check_timestamp) {
    if (i_ != value)
    {
        ExecuteUpdateValueCallbacks();
        i_ = value;
        return;
    }
    CheckTimestamp(timestamp, check_timestamp);
}

void Variable::SetValue(unsigned int value, uint64_t timestamp, bool check_timestamp) {
    if (u_ != value)
    {
        ExecuteUpdateValueCallbacks();
        u_ = value;
        return;
    }
    CheckTimestamp(timestamp, check_timestamp);
}

void Variable::SetValue(float value, uint64_t timestamp, bool check_timestamp) {
    if (f_ != value)
    {
        ExecuteUpdateValueCallbacks();
        f_ = value;
        return;
    }
    CheckTimestamp(timestamp, check_timestamp);
}

void Variable::SourceUpdate() {

}

void Variable::CheckTimestamp(uint64_t timestamp, bool check_timestamp) {
    if (check_timestamp && timestamp_ != timestamp) ExecuteUpdateValueCallbacks();
    timestamp_ = timestamp;
}

void Variable::ExecuteUpdateValueCallbacks() {
    for (const auto& callback : update_value_callbacks) {
        callback();
    }
}
