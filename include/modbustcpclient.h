#ifndef MODBUSTCPCLIENT_H
#define MODBUSTCPCLIENT_H

#pragma once

#include "variable.h"
#include <arpa/inet.h>
#include <vector>
#include <map>
#include <mutex>

struct ModbusTcpClientDeviceConfig {
    std::vector<std::string> addr;
    std::vector<int> port;
    uint max_socket;
    uint max_request;
    uint timeout;
    bool full_mapping;
    bool extended_modbus_tcp;
};

struct ModbusValue {
    uint16_t value;
    int quality;
    uint64_t timestamp;
};

struct ModbusMemory {
    std::map<uint16_t, ModbusValue> holding_registers;
};

struct ModbusVariable {
    std::string name;
    std::string type;
    uint16_t addr;
    //uint16_t bit;
};

struct ModbusRequestConfig {
    uint16_t addr;
    uint16_t len;
    std::vector<ModbusVariable> variables;
};

struct ModbusCheckRequest {
    uint8_t function;
    uint8_t addr_high_byte;
    uint8_t addr_low_byte;
    uint8_t len_high_byte;
    uint8_t len_low_byte;
    std::vector<ModbusVariable> variables;
};

using Variables = std::map<std::string, Variable>;

class ModbusTcpClient {
public:
    ModbusTcpClient(const std::string ip, const int port, const int timeout, ModbusMemory &memory, Variables &variables, std::mutex &mutex_memory, std::mutex &mutex_variables, std::mutex &mutex_transaction_id);
    ~ModbusTcpClient();
    bool Connect();
    bool CheckConnection();
    void Disconnect();
    int ReceiveResponse();
    void WriteHoldingRegisters(const ModbusRequestConfig &config, const uint16_t *value);
    void ReadHoldingRegisters(const ModbusRequestConfig &config);
    void ReadHoldingRegistersEx(const ModbusRequestConfig &config);

private:
    std::string ip_{"127.0.0.1"};
    int port_{502};
    int timeout_{5000};
    ModbusMemory &memory_;
    Variables &variables_;
    std::mutex &mutex_memory_;
    std::mutex &mutex_variables_;
    std::mutex &mutex_transaction_id_;
    bool is_connected_{false};
    uint8_t transaction_id_{1};
    int socket_{-1};
    sockaddr_in sockaddr_in_{};
    std::map<uint8_t, ModbusCheckRequest> queue_;

private:
    void IncTransactionId();
    int SendRequest(const uint8_t *request);
    void GetValue(ModbusVariable variable);
};

#endif // MODBUSTCPCLIENT_H
