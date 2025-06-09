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
    ModbusTcpClient(const std::string& ip, int port, int timeout);

    ~ModbusTcpClient();

    void WriteHoldingRegisters(const std::vector<std::vector<IndustrialProtocolUtils::DataConfig>>& config_datas, const std::vector<std::vector<uint16_t>>& data);
    void ReadHoldingRegisters(const std::vector<std::vector<IndustrialProtocolUtils::DataConfig>>& config_datas, std::vector<IndustrialProtocolUtils::DataResult>& data_result);
    void ReadHoldingRegistersEx(const std::vector<std::vector<IndustrialProtocolUtils::DataConfig>>& config_datas, std::vector<IndustrialProtocolUtils::DataResult>& data_result);
    bool Connect();
    bool CheckConnection();
    void Disconnect();

    static int GetLength(const IndustrialProtocolUtils::DataType& type);

private:
    std::string ip_{"127.0.0.1"};
    int port_{502};
    int timeout_{5000};
    modbus_t* ctx_;
    bool is_connected_{false};
    bool should_run_{false};

    IndustrialProtocolUtils::Value GetValue(const IndustrialProtocolUtils::DataType& type, const uint16_t (&data)[2]);
    void GetValue(const IndustrialProtocolUtils::DataType& type, const IndustrialProtocolUtils::Value& value, uint16_t (&data)[2]);
    void Stop();
};

#endif // MODBUSTCPCLIENT_H
