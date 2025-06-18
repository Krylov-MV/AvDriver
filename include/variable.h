#ifndef VARIABLE_H
#define VARIABLE_H

#include <string>
#include <vector>
#include <functional>
#include <memory>
#include <type_traits>

#pragma once

class Variable : public std::enable_shared_from_this<Variable>
{
public:
    Variable();

    void AddUpdateValueCallback(const std::function<void(std::shared_ptr<Variable>)>& callback) {
        update_value_callbacks.push_back(callback);
    }

    template<typename T>
    typename std::enable_if<std::is_same<T, int>::value>::type SetValue(T value, uint64_t timestamp, bool check_timestamp = false) {
        if (i_ != value) ExecuteUpdateValueCallbacks();
        i_ = value;
        CheckTimestamp(timestamp, check_timestamp);
    }

    template<typename T>
    typename std::enable_if<std::is_same<T, unsigned int>::value>::type SetValue(T value, uint64_t timestamp, bool check_timestamp = false) {
        if (u_ != value) ExecuteUpdateValueCallbacks();
        u_ = value;
        CheckTimestamp(timestamp, check_timestamp);
    }

    template<typename T>
    typename std::enable_if<std::is_same<T, float>::value>::type SetValue(T value, uint64_t timestamp, bool check_timestamp = false) {
        if (f_ != value) ExecuteUpdateValueCallbacks();
        f_ = value;
        CheckTimestamp(timestamp, check_timestamp);
    }

    void CheckTimestamp(uint64_t timestamp, bool check_timestamp) {
        if (check_timestamp && timestamp_ != timestamp) ExecuteUpdateValueCallbacks();
        timestamp_ = timestamp;
    }

private:
    void ExecuteUpdateValueCallbacks() {
        for (const auto& callback : update_value_callbacks) {
            callback(shared_from_this());
        }
    }

private:
    std::string name_{""};
    std::string type_{""};
    int i_{0};
    unsigned int u_{0};
    float f_{0};
    std::string s_{""};
    std::string quality_{""};
    uint64_t timestamp_{0};
    std::vector<std::function<void(std::shared_ptr<Variable>)>> update_value_callbacks;
};

#endif // VARIABLE_H
