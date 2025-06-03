#include "industrialprotocolutils.h"
#include <tinyxml2/tinyxml2.h>
#include <chrono>
#include <iomanip>

std::mutex file_mutex;  // Мьютекс для защиты доступа к файлу

std::vector<std::string> IndustrialProtocolUtils::Split(const std::string &str, const char delimiter) {
    std::vector<std::string> tokens;
    std::string token;

    std::stringstream ss(str);
    while (std::getline(ss, token, delimiter)) {
        tokens.push_back(token);
    }

    return tokens;
}

bool IndustrialProtocolUtils::IsIPAddress(const std::string& ip) {
    std::vector<std::string> tokens = Split(ip, '.');

    if (tokens.size() != 4) return false;
    for (const std::string& str : tokens) {
        for (const char& c : str) {
            if (!isdigit(c)) return false;
        }
        int value = stoi(str);
        if (value < 0 || value > 255) return false;
    }
    return true;
}

void IndustrialProtocolUtils::ReadConfig(IndustrialProtocolUtils::ModbusTcpDeviceConfig &modbus_tcp_device_config,
                                         std::vector<IndustrialProtocolUtils::DataConfig> &modbus_tcp_to_opc_configs,
                                         IndustrialProtocolUtils::OpcUaDeviceConfig &opc_ua_device_config,
                                         std::vector<IndustrialProtocolUtils::DataConfig> &opc_to_modbus_tcp_configs) {

    tinyxml2::XMLDocument doc;
    // Загружаем XML файл
    if (doc.LoadFile("AvDriver.xml") != tinyxml2::XML_SUCCESS) {
        std::cerr << "Ошибка при загрузке файла: " << doc.ErrorIDToName(doc.ErrorID()) << std::endl;
        return;
    }

    // Получаем корневой элемент
    std::string str = "AvDriver";
    tinyxml2::XMLElement* root = doc.RootElement();
    if (root == nullptr) {
        std::cerr << "Ошибка: корневой элемент отсутствует." << std::endl;
        return;
    }

    tinyxml2::XMLElement* programm = root->FirstChildElement("AvDriver");
    tinyxml2::XMLElement* devices = programm->FirstChildElement("devices");

    // Перебираем все элементы device
    for (tinyxml2::XMLElement* device = devices->FirstChildElement("device"); device != nullptr; device = device->NextSiblingElement("device")) {
        std::string device_name = device->FirstChildElement("name")->GetText();
        std::string device_type = device->FirstChildElement("type")->GetText();
        std::vector<std::string> device_connections;
        int device_max_socket = 1;
        int device_port = 502;
        int device_timeout = 2000;
        bool device_mapping_full_allow = true;
        if (device->FirstChildElement("settings")->FirstChildElement("max_socket")) {
            device_max_socket = device->FirstChildElement("settings")->FirstChildElement("max_socket")->IntText();
        }
        if (device->FirstChildElement("settings")->FirstChildElement("port")) {
            device_port = device->FirstChildElement("settings")->FirstChildElement("port")->IntText();
        }
        if (device->FirstChildElement("settings")->FirstChildElement("mapping_full_allow")) {
            device_mapping_full_allow = device->FirstChildElement("settings")->FirstChildElement("mapping_full_allow")->BoolText();
        }
        if (device->FirstChildElement("settings")->FirstChildElement("timeout")) {
            device_timeout = device->FirstChildElement("settings")->FirstChildElement("timeout")->IntText();
        }

        // Получаем настройки
        tinyxml2::XMLElement* settings = device->FirstChildElement("settings");
        if (settings) {
            tinyxml2::XMLElement* connections = settings->FirstChildElement("connections");
            if (connections) {
                for (tinyxml2::XMLElement* connection = connections->FirstChildElement("connection"); connection != nullptr; connection = connection->NextSiblingElement("connection")) {
                    if (connection) {
                        device_connections.push_back(connection->GetText());
                    }
                }
            }
        }

        if (device_type == "ModbusTcpClient") {
            if (device_max_socket < 0 || device_max_socket > 16) device_max_socket = 1;
            if (device_port < 0 || device_port > 65535) device_port = 502;
            if (device_timeout < 1000 || device_timeout > 32000) device_timeout = 1000;

            modbus_tcp_device_config.max_socket_in_eth = device_max_socket;
            modbus_tcp_device_config.port = device_port;
            modbus_tcp_device_config.mapping_full_allow = device_mapping_full_allow;
            modbus_tcp_device_config.timeout = device_timeout;

            for (const auto &device_connection : device_connections) {
                if (IsIPAddress(device_connection)) { modbus_tcp_device_config.ip.push_back(device_connection); }
            }
        }

        if (device_type == "OpcUaClient") {
            if (device_port < 0 || device_port > 65535) device_port = 502;

            opc_ua_device_config.port = device_port;

            if (device_connections.size() > 0) {
                if (IsIPAddress(device_connections[0])) { opc_ua_device_config.eth_osn_ip_osn = device_connections[0]; }
            }
        }
    }

    tinyxml2::XMLElement* configs = programm->FirstChildElement("configs");

    std::string config_name;
    std::string config_version;
    for (tinyxml2::XMLElement* config = configs->FirstChildElement("config"); config != nullptr; config = config->NextSiblingElement("config")) {
        config_name = config->FirstChildElement("name")->GetText();
        config_version = config->FirstChildElement("version")->GetText();
    }

    if (config_version == "0.1") {
        std::string line;
        std::ifstream file(config_name);
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
    }
}

void IndustrialProtocolUtils::Log(const std::string &log_text) {
    // Получаем текущее время
    auto now = std::chrono::steady_clock::now();

    // Преобразуем текущее время в миллисекундах с начала эпохи
    auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();

    // Получаем текущую дату и время
    std::time_t currentTime = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    std::tm* pTime = std::localtime(&currentTime);

    // Записываем лог с датой и временем с миллисекундами
    std::lock_guard<std::mutex> guard(file_mutex); // Блокируем мьютекс
    std::ofstream log_file("log.txt", std::ios::app);
    log_file << std::put_time(pTime, "%Y-%m-%d %H:%M:%S") << '.' << (milliseconds % 1000) << ' ' << log_text;
}
