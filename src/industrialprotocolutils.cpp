#include "industrialprotocolutils.h"

void IndustrialProtocolUtils::ReadConfig (IndustrialProtocolUtils::ModbusDeviceConfig &modbus_tcp_device_config,
                                          std::vector<IndustrialProtocolUtils::DataConfig> &modbus_tcp_to_opc_configs,
                                          IndustrialProtocolUtils::OpcUaDeviceConfig &opc_ua_device_config,
                                          std::vector<IndustrialProtocolUtils::DataConfig> &opc_to_modbus_tcp_configs) {
    std::string line;

    std::ifstream file("devices.txt");
    while (getline(file, line))
    {
        std::istringstream(line) >> modbus_tcp_device_config.max_socket_in_eth >> modbus_tcp_device_config.port
                                 >> modbus_tcp_device_config.eth_osn_ip_osn >> modbus_tcp_device_config.eth_osn_ip_rez
                                 >> modbus_tcp_device_config.eth_rez_ip_osn >> modbus_tcp_device_config.eth_rez_ip_rez;
    }
    file.close();

    file.open("configs.txt");
    while (getline(file, line))
    {
        std::string from_device;
        std::string from_function;
        std::string from_offset;
        std::string from_type;
        std::string from_lenght;

        std::string to_device;
        std::string to_function;
        std::string to_offset;
        std::string to_type;
        std::string to_lenght;

        std::istringstream(line) >> (from_device) >> (from_function) >> (from_offset) >> (from_type) >> (from_lenght)
                                 >> (to_device)   >> (to_function)   >> (to_offset)   >> (to_type)   >> (to_lenght);

        IndustrialProtocolUtils::DataType type;

        if(from_function == "3") { modbus_tcp_to_opc_configs.push_back({(int)std::stoi(from_offset), from_type, to_offset}); }

        if(from_function == "16") { opc_to_modbus_tcp_configs.push_back({(int)std::stoi(from_offset), from_type, to_offset}); }

        if(from_function == "23") {
            modbus_tcp_to_opc_configs.push_back({(int)std::stoi(from_offset), from_type, to_offset});
            opc_to_modbus_tcp_configs.push_back({(int)std::stoi(from_offset), from_type, to_offset + ".write"});
        }
    }
    file.close();

    std::sort(modbus_tcp_to_opc_configs.begin(), modbus_tcp_to_opc_configs.end(),
        [](const IndustrialProtocolUtils::DataConfig &a, const IndustrialProtocolUtils::DataConfig &b) { return a.offset < b.offset; });
    std::sort(opc_to_modbus_tcp_configs.begin(), opc_to_modbus_tcp_configs.end(),
        [](const IndustrialProtocolUtils::DataConfig &a, const IndustrialProtocolUtils::DataConfig &b) { return a.offset < b.offset; });

    //for (auto config : opc_to_modbus_tcp_configs) {
    //    std::cout << config.offset << " " << config.name << std::endl;
    //}
    //while(true) {}
}

float IndustrialProtocolUtils::ModbusToFloat(const uint16_t& high, const uint16_t& low) {
    union {
        float f;
        uint32_t u;
    } convert;

    convert.u = high << 16 | low;

    return convert.f;
}

int IndustrialProtocolUtils::GetLength(const IndustrialProtocolUtils::DataType& type) {
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
    default:
        return 0;
    }
}
