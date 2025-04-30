#ifndef MODBUSTCPCLIENT_H
#define MODBUSTCPCLIENT_H

#include "industrialprotocolutils.h"
#include <mutex>
#include <set>

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
        std::cout << "destructor" << std::endl;
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
    };

    struct Memory {
        std::map<uint16_t, bool> dicrete_inputs;
        std::map<uint16_t, bool> coils;
        std::map<uint16_t, uint16_t> input_registers;
        std::map<uint16_t, uint16_t> holding_registers;
    };

    struct ReadConfig {
        int offset;
        int length;
    };

    struct WriteConfig {
        int offset;
        int length;
        uint16_t value[MODBUS_MAX_WRITE_REGISTERS];
    };

    struct WriteReadConfig {
        int write_offset;
        int write_length;
        uint16_t write_value[MODBUS_MAX_WR_WRITE_REGISTERS];
        int read_offset;
        int read_length;
        uint16_t read_value[MODBUS_MAX_WR_READ_REGISTERS];
    };

    void ReadHoldingRegisters(const std::vector<ModbusTcpClient::ReadConfig>& configs,
                              std::map<uint16_t, uint16_t>& holding_registers,
                              std::mutex& mutex);

    void WriteHoldingRegisters(const std::vector<std::vector<IndustrialProtocolUtils::DataConfig>>& config_datas, const std::vector<std::vector<uint16_t>>& data);

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
