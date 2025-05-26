#ifndef MODBUSTCPCLIENT_H
#define MODBUSTCPCLIENT_H

#include "industrialprotocolutils.h"

#include <vector>
#include <modbus/modbus.h>
#include <thread>
#include <chrono>

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

    void ReadHoldingRegisters(const std::vector<std::vector<IndustrialProtocolUtils::DataConfig>>& config_datas, std::vector<IndustrialProtocolUtils::DataResult>& data_result);

    void WriteHoldingRegisters(const std::vector<std::vector<IndustrialProtocolUtils::DataConfig>>& config_datas, const std::vector<std::vector<uint16_t>>& data);

    bool CheckConnection();

    void Connect();

    bool Connect(const std::string ip, const int port);

    void Disconnect();

    static int GetLength(const IndustrialProtocolUtils::DataType& type);

private:
    std::string ip_;
    int port_;
    modbus_t* ctx_;
    bool is_connected_;
    bool should_run_;

    IndustrialProtocolUtils::Value GetValue(const IndustrialProtocolUtils::DataType& type, const uint16_t (&data)[2]);

    void GetValue(const IndustrialProtocolUtils::DataType& type, const IndustrialProtocolUtils::Value& value, uint16_t (&data)[2]);

    void Stop();
};

#endif // MODBUSTCPCLIENT_H
