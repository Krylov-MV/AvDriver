#ifndef MODBUSTCPCLIENT_H
#define MODBUSTCPCLIENT_H

#pragma once

#include "industrialprotocolutils.h"
#include <arpa/inet.h>

class ModbusTcpClient {

public:
    ModbusTcpClient(const std::string ip, const int port, const int timeout, ModbusMemory &memory, std::mutex &mutex_memory);
    ~ModbusTcpClient();
    bool Connect();
    bool CheckConnection();
    void Disconnect();
    int ReceiveResponse();
    void WriteHoldingRegisters(const ModbusClientConfig &config, const uint16_t *value);
    void ReadHoldingRegisters(const ModbusClientConfig &config);
    void ReadHoldingRegistersEx(const ModbusClientConfig &config);

private:
    std::string ip_{"127.0.0.1"};
    int port_{502};
    int timeout_{5000};
    ModbusMemory &memory_;
    std::mutex &mutex_memory_;
    bool is_connected_{false};
    bool should_run_{false};
    uint8_t transaction_id_{1};
    int socket_{-1};
    sockaddr_in sockaddr_in_{};
    std::mutex mutex_transaction_id_;
    std::map<uint8_t, ModbusCheckRequest> queue_;

private:
    void Stop();
    void IncTransactionId();
    int SendRequest(const uint8_t *request);
};

#endif // MODBUSTCPCLIENT_H
