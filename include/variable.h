#ifndef VARIABLE_H
#define VARIABLE_H

#include <string>
#include <vector>
#include <functional>
#include <memory>
#include <type_traits>

#pragma once

class Variable //: public std::enable_shared_from_this<Variable>
{
public:
    Variable();
    Variable(const std::string name, const std::string type);

    void AddUpdateValueCallback(const std::function<void()> &callback);
    void SetValue(int value, uint64_t timestamp, bool check_timestamp = false);
    void SetValue(unsigned int value, uint64_t timestamp, bool check_timestamp = false);
    void SetValue(float value, uint64_t timestamp, bool check_timestamp = false);
    void CheckTimestamp(uint64_t timestamp, bool check_timestamp);
    void SourceUpdate();

private:
    void ExecuteUpdateValueCallbacks();

private:
    std::string name_{""};
    std::string type_{""};
    int i_{0};
    unsigned int u_{0};
    float f_{0};
    std::string s_{""};
    std::string quality_{""};
    uint64_t timestamp_{0};
    std::vector<std::function<void()>> update_value_callbacks;
};

#endif // VARIABLE_H
