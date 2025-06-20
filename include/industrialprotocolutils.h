#ifndef INDUSTRIALPROTOCOLUTILS_H
#define INDUSTRIALPROTOCOLUTILS_H

#pragma once

#include "modbustcpclient.h"
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
#include <memory>

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
                /*std::vector<ModbusRequestConfig> &modbus_tcp_client_configs,
                OpcUaClientDeviceConfig &opc_ua_client_device_config,
                std::vector<OpcUaClientConfig> &opc_ua_client_configs),*/
                std::map<std::string, Variable> &all_variables,
                std::map<std::string, std::map<std::string, Variable>> &modbus_tcp_client_variables,
                std::map<std::string, std::map<std::string, Variable>> &opc_ua_client_variables);

#endif // INDUSTRIALPROTOCOLUTILS_H
