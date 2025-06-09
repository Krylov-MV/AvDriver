#ifndef INDUSTRIALPROTOCOLUTILS_H
#define INDUSTRIALPROTOCOLUTILS_H

#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <open62541/client.h>
#include <vector>
#include <algorithm>
#include <mutex>

#pragma once

class IndustrialProtocolUtils {
public:
    enum class ModbusMemoryType { DiscreteInputs, Coils, InputRegisters, HoldingRegisters };

    enum class DataType { BOOL, BYTE, INT, UINT, WORD, DINT, UDINT, DWORD, REAL, DOUBLE, STRING };

    enum class ModbusEthWorkType { ONE_ETH_OSN_OR_ONE_ETH_REZ, ONE_ETH_OSN_AND_ONE_ETH_REZ, TWO_ETH_OSN_OR_TWO_ETH_REZ, ALL_ETH };

    struct ModbusTcpDeviceConfig {
        std::vector<std::string> ip;
        uint port = 502;
        ModbusEthWorkType eth_work_type = ModbusEthWorkType::ONE_ETH_OSN_OR_ONE_ETH_REZ;
        uint max_socket_in_eth = 2;
        uint timeout_reconnect = 5000;
        uint timeout;
        bool mapping_full_allow = true;
        bool extended_modbus_tcp = true;
    };

    struct OpcUaDeviceConfig {
        std::string eth_osn_ip_osn;
        std::string eth_osn_ip_rez;
        std::string eth_rez_ip_osn;
        std::string eth_rez_ip_rez;
        uint port;
        ModbusEthWorkType eth_work_type;
        uint timeout_reconnect;
        uint timeout_read_write;
    };

    struct DataConfig {
        uint address;
        DataType type;
        std::string name;
    };

    struct Value {
        int i;
        uint u;
        float f;
    };

    struct DataResult {
        Value value;
        UA_StatusCode quality;
        DataType type;
        uint address;
        std::string name;
        UA_DateTime time_previos;
        UA_DateTime time_current;
    };

    static std::vector<std::string> Split(const std::string &str, const char delimiter);

    static bool IsIPAddress(const std::string& ip);

    static void ReadConfig(IndustrialProtocolUtils::ModbusTcpDeviceConfig &modbus_tcp_device_config, std::vector<IndustrialProtocolUtils::DataConfig> &modbus_tcp_to_opc_configs,
                            IndustrialProtocolUtils::OpcUaDeviceConfig &opc_ua_device_config, std::vector<IndustrialProtocolUtils::DataConfig> &opc_to_modbus_tcp_configs);

    static int ReadConfigXml();

    static void Log(const std::string &log_text);
};

#endif // INDUSTRIALPROTOCOLUTILS_H
