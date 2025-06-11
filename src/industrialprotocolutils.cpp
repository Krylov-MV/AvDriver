#include "industrialprotocolutils.h"

static void Log(const std::string &log_text) {
    auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();

    // Получаем текущую дату и время
    std::time_t currentTime = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    std::tm* pTime = std::localtime(&currentTime);

    // Записываем лог с датой и временем с миллисекундами
    std::lock_guard<std::mutex> guard(file_mutex); // Блокируем мьютекс
    std::ofstream log_file("log.txt", std::ios::app);
    log_file << std::put_time(pTime, "%Y-%m-%d %H:%M:%S") << '.' << (milliseconds % 1000) << ' ' << log_text;
}

static std::vector<std::string> Split(const std::string &str, const char delimiter) {
    std::vector<std::string> tokens;
    std::string token;

    std::stringstream ss(str);
    while (std::getline(ss, token, delimiter)) {
        tokens.push_back(token);
    }

    return tokens;
}

static bool IsIpAddress(const std::string& ip) {
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

void ReadConfig(ModbusTcpClientDeviceConfig &modbus_tcp_client_device_config,
                std::vector<ModbusClientConfig> &modbus_tcp_client_configs,
                OpcUaClientDeviceConfig &opc_ua_client_device_config,
                std::vector<OpcUaClientConfig> &opc_ua_client_configs) {

    tinyxml2::XMLDocument doc;
    // Загружаем XML файл
    if (doc.LoadFile("AvDriver.xml") != tinyxml2::XML_SUCCESS) {
        Log("Ошибка при загрузке файла");
        return;
    }

    // Получаем корневой элемент
    std::string str = "AvDriver";
    tinyxml2::XMLElement* root = doc.RootElement();
    if (root == nullptr) {
        Log("Ошибка: корневой элемент отсутствует");
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
        int device_max_request = 1;
        int device_port = 502;
        int device_timeout = 2000;
        bool device_mapping_full_allow = true;
        bool device_extended_modbus_tcp = false;
        if (device->FirstChildElement("settings")->FirstChildElement("max_socket")) {
            device_max_socket = device->FirstChildElement("settings")->FirstChildElement("max_socket")->IntText();
        }
        if (device->FirstChildElement("settings")->FirstChildElement("max_request")) {
            device_max_request = device->FirstChildElement("settings")->FirstChildElement("max_request")->IntText();
        }
        if (device->FirstChildElement("settings")->FirstChildElement("port")) {
            device_port = device->FirstChildElement("settings")->FirstChildElement("port")->IntText();
        }
        if (device->FirstChildElement("settings")->FirstChildElement("mapping_full_allow")) {
            device_mapping_full_allow = device->FirstChildElement("settings")->FirstChildElement("mapping_full_allow")->BoolText();
        }
        if (device->FirstChildElement("settings")->FirstChildElement("extended_modbus_tcp")) {
            device_extended_modbus_tcp = device->FirstChildElement("settings")->FirstChildElement("extended_modbus_tcp")->BoolText();
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
            if (device_max_socket < 1 || device_max_socket > 16) device_max_socket = 1;
            if (device_max_request < 1 || device_max_request > 32) device_max_request = 1;
            if (device_port < 0 || device_port > 65535) device_port = 502;
            if (device_timeout < 1000 || device_timeout > 32000) device_timeout = 1000;

            modbus_tcp_client_device_config.max_socket_in_eth = device_max_socket;
            modbus_tcp_client_device_config.port = device_port;
            modbus_tcp_client_device_config.mapping_full_allow = device_mapping_full_allow;
            modbus_tcp_client_device_config.extended_modbus_tcp = device_extended_modbus_tcp;
            modbus_tcp_client_device_config.timeout = device_timeout;

            for (const auto &device_connection : device_connections) {
                if (IsIpAddress(device_connection)) { modbus_tcp_client_device_config.addr.push_back(device_connection); }
            }
        }

        if (device_type == "OpcUaClient") {
            if (device_port < 0 || device_port > 65535) device_port = 502;

            opc_ua_client_device_config.port = device_port;

            if (device_connections.size() > 0) {
                if (IsIpAddress(device_connections[0])) { opc_ua_client_device_config.ip = device_connections[0]; }
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
}
