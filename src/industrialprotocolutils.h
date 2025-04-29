#ifndef INDUSTRIALPROTOCOLUTILS_H
#define INDUSTRIALPROTOCOLUTILS_H

#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>
#include <variant>

#pragma once

class IndustrialProtocolUtils {
public:
    enum class ModbusMemoryType { DiscreteInputs, Coils, InputRegisters, HoldingRegisters };

    enum class DataType { INT, UINT, WORD, DINT, UDINT, DWORD, REAL };

    enum class ModbusEthWorkType { ONE_ETH_OSN_OR_ONE_ETH_REZ, ONE_ETH_OSN_AND_ONE_ETH_REZ, TWO_ETH_OSN_OR_TWO_ETH_REZ, ALL_ETH };

    struct ModbusTcpDeviceConfig {
        std::string eth_osn_ip_osn = "";
        std::string eth_osn_ip_rez = "";
        std::string eth_rez_ip_osn = "";
        std::string eth_rez_ip_rez = "";
        uint port = 502;
        ModbusEthWorkType eth_work_type = ModbusEthWorkType::ONE_ETH_OSN_OR_ONE_ETH_REZ;
        uint max_socket_in_eth = 2;
        uint timeout_reconnect = 5000;
        uint timeout_read_write = 10;
        bool mapping_full_allow = true;
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
        int i = 0;
        uint u = 0;
        float f = 0;
        std::string s = "";
    };

    struct DataResult {
        Value value;
        bool quality;
        DataType type;
        uint address;
        std::string name;
        long time_previos;
        long time_current;
        uint32_t quality_previos;
        uint32_t quality_current;
    };

    static void ReadConfig (IndustrialProtocolUtils::ModbusTcpDeviceConfig &modbus_tcp_device_config, std::vector<IndustrialProtocolUtils::DataConfig> &modbus_tcp_to_opc_configs,
                            IndustrialProtocolUtils::OpcUaDeviceConfig &opc_ua_device_config, std::vector<IndustrialProtocolUtils::DataConfig> &opc_to_modbus_tcp_configs);

    static float ModbusToFloat(const uint16_t& high, const uint16_t& low);
};

#endif // INDUSTRIALPROTOCOLUTILS_H
