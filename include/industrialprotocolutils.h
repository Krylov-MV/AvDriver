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

void ReadConfig(std::map<std::string,ModbusTcpClientDeviceConfig> &modbus_tcp_client_device_configs,
                std::map<std::string,OpcUaClientDeviceConfig> &opc_ua_client_device_configs,
                std::map<std::string, Variable> &all_variables,
                std::map<std::string, Variables> &modbus_tcp_client_variables,
                std::map<std::string, std::map<std::string, std::vector<ModbusRequestConfig>>> &modbus_tcp_client_requests); //,
                //std::map<std::string, Variables> &opc_ua_client_variables);

#endif // INDUSTRIALPROTOCOLUTILS_H
