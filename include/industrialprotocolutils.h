#ifndef INDUSTRIALPROTOCOLUTILS_H
#define INDUSTRIALPROTOCOLUTILS_H

#pragma once

#include "variable.h"

#include <open62541/client.h>
#include <tinyxml2/tinyxml2.h>
#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>
#include <mutex>
#include <map>
#include <queue>
#include <chrono>
#include <iomanip>

struct ModbusTcpClientDeviceConfig {
    std::vector<std::string> addr;
    uint port;
    uint max_socket;
    uint max_request;
    uint timeout;
    bool mapping_full_allow;
    bool extended_modbus_tcp;
};

struct ModbusClientConfig {
    uint16_t addr;
    uint16_t len;
};

struct ModbusClientFullConfig {
    uint16_t addr;
    uint16_t len;
    std::map<std::string, std::shared_ptr<Variable>> variables;
};

struct ModbusValue {
    uint16_t value;
    uint8_t quality;
    uint64_t timestamp_receive;
};

struct ModbusCheckRequest {
    uint8_t function;
    uint8_t addr_high_byte;
    uint8_t addr_low_byte;
    uint8_t len_high_byte;
    uint8_t len_low_byte;
};

struct ModbusFullCheckRequest {
    uint8_t function;
    uint8_t addr_high_byte;
    uint8_t addr_low_byte;
    uint8_t len_high_byte;
    uint8_t len_low_byte;
    std::map<std::string, std::shared_ptr<Variable>> variables;
};

struct ModbusMemory {
    std::map<uint16_t, ModbusValue> holding_registers;
};

struct OpcUaClientDeviceConfig {
    std::string ip;
    uint port;
};

struct OpcUaClientConfig {
    std::string node_id;
    std::string type;
};

void Log(const std::string &log_text);

static std::vector<std::string> Split(const std::string &str, const char delimiter);

static bool IsIpAddress(const std::string &ip);

void ReadConfig(ModbusTcpClientDeviceConfig &modbus_tcp_client_device_config,
                std::vector<ModbusClientConfig> &modbus_tcp_client_configs,
                OpcUaClientDeviceConfig &opc_ua_client_device_config,
                std::vector<OpcUaClientConfig> &opc_ua_client_configs);

#endif // INDUSTRIALPROTOCOLUTILS_H
