#ifndef INDUSTRIALPROTOCOLUTILS_H
#define INDUSTRIALPROTOCOLUTILS_H

#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>
#include <variant>
#include <map>
#include <modbus/modbus.h>

#pragma once

class IndustrialProtocolUtils {
public:
    enum class ModbusMemoryType { DiscreteInputs, Coils, InputRegisters, HoldingRegisters };

    enum class DataType { INT, UINT, WORD, DINT, UDINT, DWORD, REAL };

    struct ModbusDeviceConfig {
        std::string eth_osn_ip_osn;
        std::string eth_osn_ip_rez;
        std::string eth_rez_ip_osn;
        std::string eth_rez_ip_rez;
        uint port;
        uint max_socket_in_eth;
        uint timeout_reconnect;
        uint timeout_read_write;
        bool mapping_full_allow;
    };

    struct ModbusMemory {
        std::map<uint16_t, bool> dicrete_inputs;
        std::map<uint16_t, bool> coils;
        std::map<uint16_t, uint16_t> input_registers;
        std::map<uint16_t, uint16_t> holding_registers;
    };

    struct ModbusReadConfig {
        int offset;
        int length;
        uint16_t value[MODBUS_MAX_READ_REGISTERS];
    };

    struct ModbusWriteConfig {
        int offset;
        int length;
        uint16_t value[MODBUS_MAX_WRITE_REGISTERS];
    };

    struct ModbusWriteReadConfig {
        int write_offset;
        int write_length;
        uint16_t write_value[MODBUS_MAX_WR_WRITE_REGISTERS];
        int read_offset;
        int read_length;
        uint16_t read_value[MODBUS_MAX_WR_READ_REGISTERS];
    };

    struct OpcUaDeviceConfig {
        std::string eth_osn_ip_osn;
        std::string eth_osn_ip_rez;
        std::string eth_rez_ip_osn;
        std::string eth_rez_ip_rez;
        uint port;
        uint timeout_reconnect;
        uint timeout_read_write;
    };

    struct DataConfig {
        int offset;
        std::string type;
        std::string node_id;
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
        int offset;
        std::string node_id;
        long time_previos;
        long time_current;
        uint32_t quality_previos;
        uint32_t quality_current;
    };

    struct DataFromOpc {
        std::variant<int16_t, uint16_t, int32_t, uint32_t, float> value;
        long source_timestamp;
        uint32_t quality;
    };

    static void ReadConfig (IndustrialProtocolUtils::ModbusDeviceConfig &modbus_tcp_device_config, std::vector<IndustrialProtocolUtils::DataConfig> &modbus_tcp_to_opc_configs,
                            IndustrialProtocolUtils::OpcUaDeviceConfig &opc_ua_device_config, std::vector<IndustrialProtocolUtils::DataConfig> &opc_to_modbus_tcp_configs);

    static float ModbusToFloat(const uint16_t& high, const uint16_t& low);

    static int GetLength(const IndustrialProtocolUtils::DataType& type);
};

#endif // INDUSTRIALPROTOCOLUTILS_H
