#include "modbustcpclient.h"

const std::set<std::string> ModbusTcpClient::types = { "INT", "UINT", "WORD", "DINT", "UDINT", "DWORD", "REAL" };

int ModbusTcpClient::TypeLength(const std::string& type) {
    if (!types.contains(type)) {
        return 0;
    }  else if (type == "INT" || type == "UINT" || type == "WORD") {
        return 1;
    }  else if (type == "DINT" || type == "UDINT" || type == "DWORD" || type == "REAL") {
        return 2;
    }
}

float ModbusTcpClient::ToFloat(const uint16_t& high, const uint16_t& low) {
    union {
        float f;
        uint32_t u;
    } convert;

    convert.u = high << 16 | low;

    return convert.f;
}

void ModbusTcpClient::ReadHoldingRegisters(const std::vector<ModbusTcpClient::ReadConfig>& configs,
                                           std::map<uint16_t, uint16_t>& holding_registers,
                                           std::mutex& mutex) {
    if (is_connected_) {
        if (!configs.empty()) {
            for (auto config : configs) {
                uint16_t tab_reg[MODBUS_MAX_READ_REGISTERS];

                int rc = modbus_read_registers(ctx_, config.offset, config.length, tab_reg);

                std::lock_guard<std::mutex> lock(mutex);
                if (rc == config.length) {
                    for (unsigned int i = 0; i < config.length; i++) {
                        holding_registers[config.offset + i] = tab_reg[i];
                    }
                } else {
                    //std::cerr << "Connection failed: " << modbus_strerror(errno) << std::endl;
                    for (unsigned int i = 0; i < config.length; i++) {
                        holding_registers.erase(config.offset + i);
                    }
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
                uint16_t tab_reg[MODBUS_MAX_WRITE_REGISTERS];
                int offset = config_datas[i][0].offset;
                int length = config_datas[i][config_datas[i].size() - 1].offset + IndustrialProtocolUtils::GetLength(config_datas[i][config_datas[i].size() - 1].type) - config_datas[i][0].offset;

                //std::cout << "start_address - " << start_address << " lenght - " << lenght << std::endl;
                for (unsigned long j = 0; j < data[i].size(); j++) {
                    tab_reg[j] = data[i][j];
                }

                int rc = modbus_write_registers(ctx_, offset, length, tab_reg);
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

void ModbusTcpClient::Stop() {
    should_run_ = false;
}
