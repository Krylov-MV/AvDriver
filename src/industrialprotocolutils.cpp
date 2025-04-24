#include "industrialprotocolutils.h"

void IndustrialProtocolUtils::ReadConfig (IndustrialProtocolUtils::ModbusTcpDeviceConfig &modbus_tcp_device_config,
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
        std::string from_address;
        std::string from_type;
        std::string from_lenght;

        std::string to_device;
        std::string to_function;
        std::string to_address;
        std::string to_type;
        std::string to_lenght;

        std::istringstream(line) >> (from_device) >> (from_function) >> (from_address) >> (from_type) >> (from_lenght)
                                 >> (to_device)   >> (to_function)   >> (to_address)   >> (to_type)   >> (to_lenght);

        IndustrialProtocolUtils::DataType type;

        if (from_type == "INT") {
            type = IndustrialProtocolUtils::DataType::INT;
        }
        if (from_type == "UINT") {
            type = IndustrialProtocolUtils::DataType::UINT;
        }
        if (from_type == "DINT") {
            type = IndustrialProtocolUtils::DataType::DINT;
        }
        if (from_type == "UDINT") {
            type = IndustrialProtocolUtils::DataType::UDINT;
        }
        if (from_type == "WORD") {
            type = IndustrialProtocolUtils::DataType::WORD;
        }
        if (from_type == "DWORD") {
            type = IndustrialProtocolUtils::DataType::DWORD;
        }
        if (from_type == "REAL") {
            type = IndustrialProtocolUtils::DataType::REAL;
        }

        if(from_function == "3") { modbus_tcp_to_opc_configs.push_back({(uint)std::stoi(from_address), type, to_address}); }

        if(from_function == "16") { opc_to_modbus_tcp_configs.push_back({(uint)std::stoi(from_address), type, to_address}); }

        if(from_function == "23") {
            modbus_tcp_to_opc_configs.push_back({(uint)std::stoi(from_address), type, to_address});
            opc_to_modbus_tcp_configs.push_back({(uint)std::stoi(from_address), type, to_address + ".write"});
        }
    }
    file.close();

    std::sort(modbus_tcp_to_opc_configs.begin(), modbus_tcp_to_opc_configs.end(),
        [](const IndustrialProtocolUtils::DataConfig &a, const IndustrialProtocolUtils::DataConfig &b) { return a.address < b.address; });
    std::sort(opc_to_modbus_tcp_configs.begin(), opc_to_modbus_tcp_configs.end(),
        [](const IndustrialProtocolUtils::DataConfig &a, const IndustrialProtocolUtils::DataConfig &b) { return a.address < b.address; });

    //for (auto config : opc_to_modbus_tcp_configs) {
    //    std::cout << config.address << " " << config.name << std::endl;
    //}
    //while(true) {}
}
