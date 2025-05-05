#include "modbustcpclient.h"

#include <iostream>

const std::set<std::string> ModbusTcpClient::types = { "INT", "UINT", "WORD", "DINT", "UDINT", "DWORD", "REAL" };

std::vector<ModbusTcpClient::WriteBitsConfigs> ModbusTcpClient::PrepareModbusWriteBits(const std::vector<WriteBitsConfigs>& configs, const int& max_length) {
    std::vector<WriteBitsConfigs> configs_;
    std::vector<bool> values;

    int offset = configs[0].offset;
    int length = configs[0].length;
    values.push_back(configs[0].value[0]);


    for (unsigned int i = 1; i < configs.size(); i++) {
        if (configs[i].offset - offset + configs[i].length <= max_length) {
            length = configs[i].offset - offset + configs[i].length;
            values.push_back(configs[i].value[0]);
        } else {
            configs_.push_back({offset, length, values});
            offset = configs[i].offset;
            length = configs[i].length;
        }
    }
    configs_.push_back({offset, length, values});
    return configs_;
}

std::vector<ModbusTcpClient::WriteRegistersConfig> ModbusTcpClient::PrepareModbusWriteRegisters(const std::vector<WriteRegistersConfig>& configs, const int& max_length) {
    std::vector<WriteRegistersConfig> configs_;
    std::vector<uint16_t> values;

    int offset = configs[0].offset;
    int length = configs[0].length;
    for (const uint16_t& v : configs[0].value) {
        values.push_back(v);
    }

    for (unsigned int i = 1; i < configs.size(); i++) {
        if (configs[i].offset - offset + configs[i].length <= max_length) {
            length = configs[i].offset - offset + configs[i].length;
            for (const uint16_t& v : configs[i].value) {
                values.push_back(v);
            }
        } else {
            configs_.push_back({offset, length, values});
            offset = configs[i].offset;
            length = configs[i].length;
            values.clear();
            for (const uint16_t& v : configs[i].value) {
                values.push_back(v);
            }
        }
    }
    configs_.push_back({offset, length, values});
    return configs_;
}

std::vector<ModbusTcpClient::ReadConfig> ModbusTcpClient::PrepareModbusRead(const std::vector<ReadConfig>& configs, const int& max_length) {
    std::vector<ModbusTcpClient::ReadConfig> configs_;

    int offset = configs[0].offset;
    int length = configs[0].length;

    for (unsigned int i = 1; i < configs.size(); i++) {
        if (configs[i].offset - offset + configs[i].length <= max_length) {
            length = configs[i].offset - offset + configs[i].length;
        } else {
            configs_.push_back({offset, length});
            offset = configs[i].offset;
            length = configs[i].length;
        }
    }
    configs_.push_back({offset, length});
    return configs_;
}

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

void ModbusTcpClient::WriteCoils(const std::vector<WriteBitsConfigs>& configs) {
    if (is_connected_) {
        if (!configs.empty()) {
            for (auto config : configs) {
                uint8_t value[MODBUS_MAX_WRITE_BITS];
                for (int i = 0; i < config.value.size(); i++) { value[i] = config.value[i]; }
                int rc = modbus_write_bits(ctx_, config.offset, config.length, value);

                if (rc != config.length) {
                    //std::cerr << "Connection failed: " << modbus_strerror(errno) << std::endl;
                    Disconnect();
                }
            }
        }
    }
}

void ModbusTcpClient::WriteHoldingRegisters(const std::vector<WriteRegistersConfig>& configs) {
    if (is_connected_) {
        if (!configs.empty()) {
            for (auto config : configs) {
                uint16_t value[MODBUS_MAX_READ_REGISTERS];
                for (int i = 0; i < config.value.size(); i++) { value[i] = config.value[i]; }
                int rc = modbus_write_registers(ctx_, config.offset, config.length, value);

                if (rc != config.length) {
                    //std::cerr << "Connection failed: " << modbus_strerror(errno) << std::endl;
                    Disconnect();
                }
            }
        }
    }
}

void ModbusTcpClient::ReadDiscreteInputs(const std::vector<ReadConfig>& configs,
                                           std::map<uint16_t, bool>& result,
                                           std::mutex& mutex) {
    if (is_connected_) {
        if (!configs.empty()) {
            for (auto config : configs) {
                uint8_t tab_reg[MODBUS_MAX_READ_BITS];

                int rc = modbus_read_input_bits(ctx_, config.offset, config.length, tab_reg);

                std::lock_guard<std::mutex> lock(mutex);
                if (rc == config.length) {
                    for (unsigned int i = 0; i < config.length; i++) {
                        result[config.offset + i] = tab_reg[i];
                    }
                } else {
                    //std::cerr << "Connection failed: " << modbus_strerror(errno) << std::endl;
                    for (unsigned int i = 0; i < config.length; i++) {
                        result.erase(config.offset + i);
                    }
                    Disconnect();
                }
            }
        }
    }
}

void ModbusTcpClient::ReadCoils(const std::vector<ReadConfig>& configs,
                                           std::map<uint16_t, bool>& result,
                                           std::mutex& mutex) {
    if (is_connected_) {
        if (!configs.empty()) {
            for (auto config : configs) {
                uint8_t tab_reg[MODBUS_MAX_READ_BITS];

                int rc = modbus_read_bits(ctx_, config.offset, config.length, tab_reg);

                std::lock_guard<std::mutex> lock(mutex);
                if (rc == config.length) {
                    for (unsigned int i = 0; i < config.length; i++) {
                        result[config.offset + i] = tab_reg[i];
                    }
                } else {
                    //std::cerr << "Connection failed: " << modbus_strerror(errno) << std::endl;
                    for (unsigned int i = 0; i < config.length; i++) {
                        result.erase(config.offset + i);
                    }
                    Disconnect();
                }
            }
        }
    }
}

void ModbusTcpClient::ReadInputRegisters(const std::vector<ReadConfig>& configs,
                                           std::map<uint16_t, uint16_t>& result,
                                           std::mutex& mutex) {
    if (is_connected_) {
        if (!configs.empty()) {
            for (auto config : configs) {
                uint16_t tab_reg[MODBUS_MAX_READ_REGISTERS];

                int rc = modbus_read_input_registers(ctx_, config.offset, config.length, tab_reg);

                std::lock_guard<std::mutex> lock(mutex);
                if (rc == config.length) {
                    for (unsigned int i = 0; i < config.length; i++) {
                        result[config.offset + i] = tab_reg[i];
                    }
                } else {
                    //std::cerr << "Connection failed: " << modbus_strerror(errno) << std::endl;
                    for (unsigned int i = 0; i < config.length; i++) {
                        result.erase(config.offset + i);
                    }
                    Disconnect();
                }
            }
        }
    }
}

void ModbusTcpClient::ReadHoldingRegisters(const std::vector<ReadConfig>& configs,
                                           std::map<uint16_t, uint16_t>& result,
                                           std::mutex& mutex) {
    if (is_connected_) {
        if (!configs.empty()) {
            for (auto config : configs) {
                uint16_t tab_reg[MODBUS_MAX_READ_REGISTERS];

                int rc = modbus_read_registers(ctx_, config.offset, config.length, tab_reg);
                std::lock_guard<std::mutex> lock(mutex);
                if (rc == config.length) {
                    for (unsigned int i = 0; i < config.length; i++) {
                        result[config.offset + i] = tab_reg[i];
                    }
                } else {
                    //std::cerr << "Connection failed: " << modbus_strerror(errno) << std::endl;
                    for (unsigned int i = 0; i < config.length; i++) {
                        result.erase(config.offset + i);
                    }
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
