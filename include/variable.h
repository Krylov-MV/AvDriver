#ifndef VARIABLE_H
#define VARIABLE_H

#include <string>
#include <vector>
#include <functional>
#include <memory>

#pragma once

class Variable : public std::enable_shared_from_this<Variable>
{
public:
    Variable();
    void AddUpdateValueCallback(const std::function<void(std::shared_ptr<Variable>)>& callback) {
        update_value_callbacks.push_back(callback);
    }

private:
    void ExecuteUpdateValueCallbacks() {
        for (const auto& callback : update_value_callbacks) {
            callback(shared_from_this());
        }
    }

private:
    std::string name{""};
    std::string type{""};
    int i{0};
    unsigned int u{0};
    float f{0};
    std::string s{""};
    std::string quality{""};
    uint64_t timestamp_receive{0};
    std::vector<std::function<void(std::shared_ptr<Variable>)>> update_value_callbacks;
    std::string source_area;
    uint16_t source_addr;
    std::string source_node;
};

#endif // VARIABLE_H
