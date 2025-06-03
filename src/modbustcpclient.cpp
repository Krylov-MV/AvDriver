#include "modbustcpclient.h"

void ModbusTcpClient::ReadHoldingRegisters(const std::vector<std::vector<IndustrialProtocolUtils::DataConfig>>& config_datas, std::vector<IndustrialProtocolUtils::DataResult>& data_result) {
    if (is_connected_) {
        if (!config_datas.empty()) {
            data_result.clear();

            for (auto config_data : config_datas) {
                uint16_t tab_reg[125];
                int start_address = config_data[0].address;
                int length = config_data[config_data.size() - 1].address + GetLength(config_data[config_data.size() - 1].type) - config_data[0].address;

                if (is_connected_){
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
}

void ModbusTcpClient::WriteHoldingRegisters(const std::vector<std::vector<IndustrialProtocolUtils::DataConfig>>& config_datas, const std::vector<std::vector<uint16_t>>& data) {
    if (is_connected_) {
        if (!config_datas.empty()) {
            for (unsigned long i = 0; i < config_datas.size(); i++) {
                uint16_t tab_reg[100];
                int start_address = config_datas[i][0].address;
                int lenght = config_datas[i][config_datas[i].size() - 1].address + GetLength(config_datas[i][config_datas[i].size() - 1].type) - config_datas[i][0].address;

                //std::cout << "start_address - " << start_address << " lenght - " << lenght << std::endl;
                for (unsigned long j = 0; j < data[i].size(); j++) {
                    tab_reg[j] = data[i][j];
                }

                if (is_connected_) {
                    int rc = modbus_write_registers(ctx_, start_address, lenght, tab_reg);
                    //std::cout << start_address << std::endl;
                    if (rc < 0) {
                        //std::cout << "start_address - " << start_address << " lenght - " << lenght << std::endl;
                        Disconnect();
                    }
                }
            }
        }
    }
}

bool ModbusTcpClient::CheckConnection() {
    return is_connected_;
}

bool ModbusTcpClient::Connect() {
    ctx_ = modbus_new_tcp(ip_.c_str(), port_);
    uint32_t timeout_sec = timeout_ / 1000;
    uint32_t timeout_usec = timeout_ % 1000 * 1000;
    modbus_set_response_timeout(ctx_, timeout_sec, timeout_usec);

    if (modbus_connect(ctx_) == -1) {
        is_connected_ = false;
        modbus_close(ctx_);
        modbus_free(ctx_);
    } else {
        is_connected_ = true;
    }

    return is_connected_;
}

void ModbusTcpClient::Disconnect() {
    is_connected_ = false;
    modbus_close(ctx_);
    modbus_free(ctx_);
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
        return (IndustrialProtocolUtils::Value) {.i = data[0], .u = 0, .f = 0};
    case IndustrialProtocolUtils::DataType::UINT:
    case IndustrialProtocolUtils::DataType::WORD:
        return (IndustrialProtocolUtils::Value) {.i = 0, .u = data[0], .f = 0};
    case IndustrialProtocolUtils::DataType::DINT:
        return (IndustrialProtocolUtils::Value) {.i = (data[1] << 16) + data[0], .u = 0, .f = 0};
    case IndustrialProtocolUtils::DataType::UDINT:
    case IndustrialProtocolUtils::DataType::DWORD:
        return (IndustrialProtocolUtils::Value) {.i = 0, .u = (uint)(data[1] << 16) + data[0], .f = 0};
    case IndustrialProtocolUtils::DataType::REAL:
        return (IndustrialProtocolUtils::Value) {.i = 0, .u = 0, .f = modbus_get_float_cdab(data)};
    default:
        return (IndustrialProtocolUtils::Value) {.i = 0, .u = 0, .f = 0};
    }
}

void ModbusTcpClient::GetValue(const IndustrialProtocolUtils::DataType& type, const IndustrialProtocolUtils::Value& value, uint16_t (&data)[2]) {
    switch (type)
    {
    case IndustrialProtocolUtils::DataType::INT:
        data[0] = value.i;
        break;
    case IndustrialProtocolUtils::DataType::UINT:
    case IndustrialProtocolUtils::DataType::WORD:
        data[0] = value.u;
        break;
    case IndustrialProtocolUtils::DataType::DINT:
        data[0] = value.i >> 16;
        data[1] = value.i & 65535;
        break;
    case IndustrialProtocolUtils::DataType::UDINT:
    case IndustrialProtocolUtils::DataType::DWORD:
        data[0] = value.u >> 16;
        data[1] = value.u & 65535;
        break;
    case IndustrialProtocolUtils::DataType::REAL:
        modbus_set_float_abcd(value.f, &data[0]);
        break;
    default:
        data[0] = 0;
        data[1] = 0;
        break;
    }
}

void ModbusTcpClient::Stop() {
    should_run_ = false;
}
