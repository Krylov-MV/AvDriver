#ifndef MODBUSTCPCLIENT_H
#define MODBUSTCPCLIENT_H

#include <modbus/modbus.h>
#include <mutex>
#include <set>
#include <vector>
#include <map>

#pragma once

class ModbusTcpClient {
public:
    ModbusTcpClient(const std::string& ip, int port)
        : ip_(ip), port_(port), is_connected_(false), should_run_(true) {
        Connect();
    }
    ModbusTcpClient()
        : is_connected_(false), should_run_(true) {
    }

    ~ModbusTcpClient() {
        should_run_ = false;
        modbus_free(ctx_);
    }

    enum class ModbusMemoryType { DiscreteInputs, Coils, InputRegisters, HoldingRegisters };

    struct DeviceConfig {
        std::string eth_osn_ip_osn;
        std::string eth_osn_ip_rez;
        std::string eth_rez_ip_osn;
        std::string eth_rez_ip_rez;
        uint port;
        uint max_socket_in_eth;
        uint timeout_reconnect;
        uint timeout_read_write;
        bool mapping_full_allow;
        bool write_read_registers_allow;
    };

    struct Memory {
        std::map<uint16_t, bool> discrete_inputs;
        std::map<uint16_t, bool> coils;
        std::map<uint16_t, uint16_t> input_registers;
        std::map<uint16_t, uint16_t> holding_registers;
    };

    struct WriteRegistersConfig {
        int offset;
        int length;
        std::vector<uint16_t> value;
    };

    struct WriteBitsConfigs {
        int offset;
        int length;
        std::vector<bool> value;
    };

    struct ReadConfig {
        int offset;
        int length;
    };

    struct WriteConfigs {
        std::vector<WriteBitsConfigs> coils;
        std::vector<WriteRegistersConfig> holding_registers;
    };

    struct ReadConfigs {
        std::vector<ReadConfig> discrete_inputs;
        std::vector<ReadConfig> coils;
        std::vector<ReadConfig> input_registers;
        std::vector<ReadConfig> holding_registers;
    };

    struct WriteReadConfigs {
        WriteConfigs write;
        ReadConfigs read;
    };

    static std::vector<ModbusTcpClient::WriteBitsConfigs> PrepareModbusWriteBits(const std::vector<ModbusTcpClient::WriteBitsConfigs>& configs, const int& max_length);

    static std::vector<ModbusTcpClient::WriteRegistersConfig> PrepareModbusWriteRegisters(const std::vector<ModbusTcpClient::WriteRegistersConfig>& configs, const int& max_length);

    static std::vector<ModbusTcpClient::ReadConfig> PrepareModbusRead(const std::vector<ModbusTcpClient::ReadConfig>& configs, const int& max_length);

    void WriteCoils(const std::vector<WriteBitsConfigs>& configs);

    void WriteHoldingRegisters(const std::vector<WriteRegistersConfig>& configs);

    void ReadDiscreteInputs(const std::vector<ModbusTcpClient::ReadConfig>& configs,
                              std::map<uint16_t, bool>& result,
                              std::mutex& mutex);

    void ReadCoils(const std::vector<ModbusTcpClient::ReadConfig>& configs,
                              std::map<uint16_t, bool>& result,
                              std::mutex& mutex);

    void ReadInputRegisters(const std::vector<ModbusTcpClient::ReadConfig>& configs,
                              std::map<uint16_t, uint16_t>& result,
                              std::mutex& mutex);

    void ReadHoldingRegisters(const std::vector<ModbusTcpClient::ReadConfig>& configs,
                              std::map<uint16_t, uint16_t>& result,
                              std::mutex& mutex);

    static int TypeLength(const std::string& type);

    bool CheckConnection();

    void Connect();

    bool Connect(std::string ip, int port);

    void Disconnect();

    static float ToFloat(const uint16_t& high, const uint16_t& low);

private:
    std::string ip_;
    int port_;
    modbus_t* ctx_;
    bool is_connected_;
    bool should_run_;
    static const std::set<std::string> types;

    void Stop();
};


#endif // MODBUSTCPCLIENT_H
