#ifndef VARIABLE_H
#define VARIABLE_H

#pragma once

#include <string>
#include <vector>
#include <functional>
#include <variant>

using Value = std::variant<int, float, std::string>;
using Callback = std::function<void(Value &value)>;

class Variable
{
public:
    Variable();
    Variable(const std::string name, const std::string type);
    Variable(const std::string name, const std::string type, std::string area, uint16_t addr);
    //Variable(const std::string name, const std::string type, std::string area, uint16_t addr, uint16_t bit);
    Variable(const std::string name, const std::string type, std::string node);

    void AddUpdateValueCallback(Callback callback);
    void SetValue(Value &value, uint64_t &timestamp, int &quality, bool check_timestamp = false);
    void CheckTimestamp(uint64_t timestamp, bool check_timestamp);
    void SourceUpdate(Value &value);

private:
    void ExecuteUpdateValueCallbacks();

private:
    std::string name_{""};
    std::string type_{""};
    std::string area_{""};
    uint16_t addr_{0};
    //uint16_t bit_{0};
    std::string node_{};
    Value value_;
    uint8_t quality_{0};
    uint64_t timestamp_{0};
    std::vector<Callback> update_value_callbacks_;
};

#endif // VARIABLE_H
