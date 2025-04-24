#include "modbustcpclient.h"

void ModbusTcpClient::ReadHoldingRegisters(const std::vector<std::vector<IndustrialProtocolUtils::DataConfig>>& config_datas, std::vector<IndustrialProtocolUtils::DataResult>& data_result) {
    if (is_connected_) {
        if (!config_datas.empty()) {
            data_result.clear();

            for (auto config_data : config_datas) {
                uint16_t tab_reg[125];
                int start_address = config_data[0].address;
                int length = config_data[config_data.size() - 1].address + GetLength(config_data[config_data.size() - 1].type) - config_data[0].address;

                int rc = modbus_read_registers(ctx_, start_address, length, tab_reg);

                if (rc == length) {
                    unsigned int j = 0;
                    while (j < config_data.size()) {
                        uint16_t data[2];
                        IndustrialProtocolUtils::DataResult result;
                        data[0] = tab_reg[config_data[j].address - start_address];
                        if (GetLength(config_data[j].type) == 2) { data[1] = tab_reg[config_data[j].address - start_address + 1]; }
                        result.name = config_data[j].name;
                        result.type = config_data[j].type;
                        result.quality = true;
                        result.value = GetValue(result.type, data);
                        result.address = config_data[j].address;
                        data_result.push_back(result);
                        j++;
                    }
                } else {
                    Disconnect();
                }
            }
        }
    }
}

void ModbusTcpClient::WriteHoldingRegisters(const std::vector<std::vector<IndustrialProtocolUtils::DataConfig>>& config_datas, const std::vector<std::vector<uint16_t>>& data) {
    if (is_connected_) {
        if (!config_datas.empty()) {
            for (unsigned long i = 0; i < config_datas.size(); i++) {
                uint16_t tab_reg[100];
                int start = config_datas[i][0].address;
                int lenght = config_datas[i][config_datas[i].size() - 1].address + GetLength(config_datas[i][config_datas[i].size() - 1].type) - config_datas[i][0].address;

                for (unsigned long j = 0; j < data[i].size(); j++) {
                    tab_reg[j] = data[i][j];
                }

                int rc = modbus_write_registers(ctx_, start, lenght, tab_reg);
                if (rc < 0) {
                    Disconnect();
                }
            }
        }
    }
}

bool ModbusTcpClient::CheckConnection() {
    return is_connected_;
}

void ModbusTcpClient::Connect() {
    ctx_ = modbus_new_tcp(ip_.c_str(), port_);
    if (modbus_connect(ctx_) == -1) {
        is_connected_ = false;
    } else {
        is_connected_ = true;
    }
}

bool ModbusTcpClient::Connect(std::string ip, int port) {
    ip_ = ip;
    port_ = port;
    ctx_ = modbus_new_tcp(ip_.c_str(), port_);
    if (modbus_connect(ctx_) == -1) {
        is_connected_ = false;
        return false;
    } else {
        is_connected_ = true;
        return true;
    }
}

void ModbusTcpClient::Disconnect() {
    if (is_connected_) {
        modbus_close(ctx_);
        is_connected_ = false;
    }
}

int ModbusTcpClient::GetLength(const IndustrialProtocolUtils::DataType& type) {
    switch (type)
    {
    case IndustrialProtocolUtils::DataType::INT:
    case IndustrialProtocolUtils::DataType::UINT:
    case IndustrialProtocolUtils::DataType::WORD:
        return 1;
    case IndustrialProtocolUtils::DataType::DINT:
    case IndustrialProtocolUtils::DataType::UDINT:
    case IndustrialProtocolUtils::DataType::DWORD:
    case IndustrialProtocolUtils::DataType::REAL:
        return 2;
    case IndustrialProtocolUtils::DataType::DOUBLE:
        return 4;
    default:
        return 0;
    }
}

IndustrialProtocolUtils::Value ModbusTcpClient::GetValue(const IndustrialProtocolUtils::DataType& type, const uint16_t (&data)[2]) {
    switch (type)
    {
    case IndustrialProtocolUtils::DataType::INT:
        return (IndustrialProtocolUtils::Value) {data[0], 0, 0};
    case IndustrialProtocolUtils::DataType::UINT:
    case IndustrialProtocolUtils::DataType::WORD:
        return (IndustrialProtocolUtils::Value) {0, data[0], 0};
    case IndustrialProtocolUtils::DataType::DINT:
        return (IndustrialProtocolUtils::Value) {(data[0] << 16) + data[1], 0, 0};
    case IndustrialProtocolUtils::DataType::UDINT:
    case IndustrialProtocolUtils::DataType::DWORD:
        return (IndustrialProtocolUtils::Value) {0, (uint)(data[0] << 16) + data[1], 0};
    case IndustrialProtocolUtils::DataType::REAL:
        return (IndustrialProtocolUtils::Value) {0, 0, modbus_get_float_cdab(data)};
    default:
        return (IndustrialProtocolUtils::Value) {0, 0, 0};
    }
}

void ModbusTcpClient::GetValue(const IndustrialProtocolUtils::DataType& type, const IndustrialProtocolUtils::Value& value, uint16_t (&data)[2]) {
    switch (type)
    {
    case IndustrialProtocolUtils::DataType::INT:
        data[0] = value.i;
    case IndustrialProtocolUtils::DataType::UINT:
    case IndustrialProtocolUtils::DataType::WORD:
        data[0] = value.u;
    case IndustrialProtocolUtils::DataType::DINT:
        data[0] = value.i >> 16;
        data[1] = value.i & 65535;
    case IndustrialProtocolUtils::DataType::UDINT:
    case IndustrialProtocolUtils::DataType::DWORD:
        data[0] = value.u >> 16;
        data[1] = value.u & 65535;
    case IndustrialProtocolUtils::DataType::REAL:
        modbus_set_float_abcd(value.f, &data[0]);
    default:
        data[0] = 0;
        data[1] = 0;
    }
}

void ModbusTcpClient::Stop() {
    should_run_ = false;
}
