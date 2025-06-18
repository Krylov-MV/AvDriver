#include "industrialprotocolutils.h"

using namespace tinyxml2;

void Log(const std::string &log_text) {
    std::mutex file_mutex;

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

/*void ReadConfig(ModbusTcpClientDeviceConfig &modbus_tcp_client_device_config,
                std::vector<ModbusClientConfig> &modbus_tcp_client_configs,
                OpcUaClientDeviceConfig &opc_ua_client_device_config,
                std::vector<OpcUaClientConfig> &opc_ua_client_configs) {

    XMLDocument doc;
    // Загружаем XML файл
    if (doc.LoadFile("AvDriver.xml") != XML_SUCCESS) {
        Log("Ошибка при загрузке файла");
        return;
    }

    // Получаем корневой элемент
    XMLElement* root = doc.RootElement();
    if (root == nullptr) {
        Log("Ошибка: корневой элемент отсутствует");
        return;
    }

    XMLElement* programm = root->FirstChildElement("AvDriver");
    XMLElement* devices = programm->FirstChildElement("devices");

    // Перебираем все элементы device
    for (XMLElement* device = devices->FirstChildElement("device"); device != nullptr; device = device->NextSiblingElement("device")) {
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
        XMLElement* settings = device->FirstChildElement("settings");
        if (settings) {
            XMLElement* connections = settings->FirstChildElement("connections");
            if (connections) {
                for (XMLElement* connection = connections->FirstChildElement("connection"); connection != nullptr; connection = connection->NextSiblingElement("connection")) {
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

            modbus_tcp_client_device_config.max_socket = device_max_socket;
            modbus_tcp_client_device_config.max_request = device_max_request;
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

    XMLElement* configs = programm->FirstChildElement("configs");

    std::string config_name;
    std::string config_version;
    for (XMLElement* config = configs->FirstChildElement("config"); config != nullptr; config = config->NextSiblingElement("config")) {
        config_name = config->FirstChildElement("name")->GetText();
        config_version = config->FirstChildElement("version")->GetText();
    }

    if (config_version == "0.2") {
        // Загружаем XML файл
        if (doc.LoadFile(config_name.c_str()) != XML_SUCCESS) {
            Log("Ошибка при загрузке файла конфигурации");
            return;
        }

        // Получаем корневой элемент
        root = doc.RootElement();
        if (root == nullptr) {
            Log("Ошибка: корневой элемент отсутствует");
            return;
        }

        programm = root->FirstChildElement("AlphaServer");
        XMLElement* variables = programm->FirstChildElement("variables");

        // Перебираем все элементы device
        for (XMLElement* variable = variables->FirstChildElement("variable"); variable != nullptr; variable = variable->NextSiblingElement("variable")) {
            std::string variable_name;
            std::string variable_type;
            std::string variable_source_name;
            std::string variable_source_area;
            std::string variable_source_addr;
            std::string variable_source_node;
            std::string variable_transfer_name;
            std::string variable_transfer_addr;
            std::string variable_transfer_node;

            if (!variable->FirstChildElement("name") || !variable->FirstChildElement("type")) {
                continue;
            }
            variable_name = variable->FirstChildElement("name")->GetText();
            variable_type = variable->FirstChildElement("type")->GetText();

            if (variable->FirstChildElement("source")) {
                XMLElement* source = variable->FirstChildElement("source");

                if (source->FirstChildElement("name") && source->FirstChildElement("area") && source->FirstChildElement("addr") && !source->FirstChildElement("node")) {
                    variable_source_name = source->FirstChildElement("name")->GetText();
                    variable_source_area = source->FirstChildElement("area")->GetText();
                    variable_source_addr = source->FirstChildElement("addr")->GetText();
                    variable_source_node = "";
                }
                if (source->FirstChildElement("name") && !source->FirstChildElement("area") && !source->FirstChildElement("addr") && source->FirstChildElement("node")) {
                    variable_source_name = source->FirstChildElement("name")->GetText();
                    variable_source_area = "";
                    variable_source_addr = "";
                    variable_source_node = source->FirstChildElement("node")->GetText();
                }
            }

            if (variable->FirstChildElement("transfers")) {
                XMLElement* transfers = variable->FirstChildElement("transfers");

                for (XMLElement* transfer = transfers->FirstChildElement("transfer"); variable != nullptr; transfer = transfer->NextSiblingElement("transfer")) {
                    if (transfer->FirstChildElement("name") && transfer->FirstChildElement("addr") && !transfer->FirstChildElement("node")) {
                        variable_transfer_name = transfer->FirstChildElement("name")->GetText();
                        variable_transfer_addr = transfer->FirstChildElement("addr")->GetText();
                        variable_transfer_node = "";
                    }
                    if (transfer->FirstChildElement("name") && !transfer->FirstChildElement("addr") && transfer->FirstChildElement("node")) {
                        variable_transfer_name = transfer->FirstChildElement("name")->GetText();
                        variable_transfer_addr = "";
                        variable_transfer_node = transfer->FirstChildElement("node")->GetText();
                    }
                }
            }
        }
    }
}*/
