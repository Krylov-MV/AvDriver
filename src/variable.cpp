#include "variable.h"

Variable::Variable(){}

Variable::Variable(const std::string name, const std::string type) : name_(name), type_(type) {}

Variable::Variable(const std::string name, const std::string type, std::string area, uint16_t addr) : name_(name), type_(type), area_(area), addr_(addr) {}

//Variable::Variable(const std::string name, const std::string type, std::string area, uint16_t addr, uint16_t bit) : name_(name), type_(type), area_(area), addr_(addr), bit_(bit) {}

Variable::Variable(const std::string name, const std::string type, std::string node) : name_(name), type_(type), node_(node) {}

void Variable::AddUpdateValueCallback(Callback callback) {
    update_value_callbacks_.push_back(callback);
}

void Variable::SetValue(Value &value, uint64_t &timestamp, int &quality, bool check_timestamp) {
    if (value_ != value)
    {
        ExecuteUpdateValueCallbacks();
        value_ = value;
        return;
    }
    CheckTimestamp(timestamp, check_timestamp);
}

void Variable::SourceUpdate(Value &value) {
    value_ = value;
}

void Variable::CheckTimestamp(uint64_t timestamp, bool check_timestamp) {
    if (check_timestamp && timestamp_ != timestamp) ExecuteUpdateValueCallbacks();
    timestamp_ = timestamp;
}

void Variable::ExecuteUpdateValueCallbacks() {
    for (const auto& callback : update_value_callbacks_) {
        callback(value_);
    }
}
