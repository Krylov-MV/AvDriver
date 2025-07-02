#include "industrialprotocolutils.h"
#include "modbustcpclient.h"
#include <algorithm>

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

void ReadConfig(std::map<std::string,ModbusTcpClientDeviceConfig> &modbus_tcp_client_device_configs,
                std::map<std::string,OpcUaClientDeviceConfig> &opc_ua_client_device_configs,
                std::map<std::string, Variable> &all_variables,
                std::map<std::string, Variables> &modbus_tcp_client_variables,
                std::map<std::string, std::map<std::string, std::vector<ModbusRequestConfig>>> &modbus_tcp_client_requests) {//,
                //std::map<std::string, Variables> &opc_ua_client_variables) {

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
        int device_timeout = 2000;
        bool device_full_mapping = true;
        bool device_extended_modbus_tcp = false;
        if (device->FirstChildElement("settings")->FirstChildElement("max_socket")) {
            device_max_socket = device->FirstChildElement("settings")->FirstChildElement("max_socket")->IntText();
        }
        if (device->FirstChildElement("settings")->FirstChildElement("max_request")) {
            device_max_request = device->FirstChildElement("settings")->FirstChildElement("max_request")->IntText();
        }
        if (device->FirstChildElement("settings")->FirstChildElement("mapping_full_allow")) {
            device_full_mapping = device->FirstChildElement("settings")->FirstChildElement("full_mapping")->BoolText();
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
            if (device_timeout < 1000 || device_timeout > 32000) device_timeout = 1000;

            modbus_tcp_client_device_configs[device_name].max_socket = device_max_socket;
            modbus_tcp_client_device_configs[device_name].max_request = device_max_request;
            modbus_tcp_client_device_configs[device_name].full_mapping = device_full_mapping;
            modbus_tcp_client_device_configs[device_name].extended_modbus_tcp = device_extended_modbus_tcp;
            modbus_tcp_client_device_configs[device_name].timeout = device_timeout;

            for (const auto &device_connection : device_connections) {
                std::vector<std::string> addr = Split(device_connection, ':');
                if (addr.size() == 2) {
                    std::string ip = addr[0];
                    int port = std::stoi(addr[1]);
                    if (IsIpAddress(ip)) {
                        if (port > 0 && port <= 65535) {
                            modbus_tcp_client_device_configs[device_name].addr.push_back(ip);
                            modbus_tcp_client_device_configs[device_name].port.push_back(port);
                        }
                    }
                }
            }
        }

        if (device_type == "OpcUaClient") {
            for (const auto &device_connection : device_connections) {
                std::vector<std::string> addr = Split(device_connection, ':');
                if (addr.size() == 2) {
                    std::string ip = addr[0];
                    int port = std::stoi(addr[1]);
                    if (IsIpAddress(ip)) {
                        if (port > 0 && port <= 65535) {
                            opc_ua_client_device_configs[device_name].ip = ip;
                            opc_ua_client_device_configs[device_name].port = port;
                        }
                    }
                }
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

        programm = root->FirstChildElement("AvServer");
        XMLElement* variables = programm->FirstChildElement("variables");

        std::map<std::string, std::map<std::string, std::vector<ModbusVariable>>> temp_modbus_tcp_client_variables;

        // Перебираем все элементы device
        for (XMLElement* variable = variables->FirstChildElement("variable"); variable != nullptr; variable = variable->NextSiblingElement("variable")) {
            std::string variable_name;
            std::string variable_type;
            std::string variable_source_name;
            std::string variable_source_area;
            uint16_t variable_source_addr;
            uint16_t variable_source_bit;
            std::string variable_source_node;
            std::string variable_transfer_name;
            uint16_t variable_transfer_addr;
            std::string variable_transfer_node;

            if (!variable->FirstChildElement("name") || !variable->FirstChildElement("type")) {
                continue;
            }
            variable_name = variable->FirstChildElement("name")->GetText();
            variable_type = variable->FirstChildElement("type")->GetText();

            all_variables[variable_name] = {variable_name, variable_type};

            if (variable->FirstChildElement("source")) {
                XMLElement* source = variable->FirstChildElement("source");

                if (source->FirstChildElement("name") && source->FirstChildElement("area") && source->FirstChildElement("addr") && !source->FirstChildElement("node")) {
                    variable_source_name = source->FirstChildElement("name")->GetText();
                    variable_source_area = source->FirstChildElement("area")->GetText();
                    variable_source_addr = source->FirstChildElement("addr")->IntText();
                    //variable_source_bit = source->FirstChildElement("bit")->IntText();

                    modbus_tcp_client_variables[variable_source_name][variable_name].AddUpdateValueCallback([&, variable_name] (Value &value) {all_variables[variable_name].SourceUpdate(value);});

                    temp_modbus_tcp_client_variables[variable_source_name][variable_source_area].push_back({variable_name, variable_type, variable_source_addr});//, variable_source_bit});
                }
                /*if (source->FirstChildElement("name") && !source->FirstChildElement("area") && !source->FirstChildElement("addr") && source->FirstChildElement("node")) {
                    variable_source_name = source->FirstChildElement("name")->GetText();
                    variable_source_node = source->FirstChildElement("node")->GetText();

                    opc_ua_client_variables[variable_source_name][variable_name] = {variable_name, variable_type, variable_source_node};

                    opc_ua_client_variables[variable_source_name][variable_name].AddUpdateValueCallback([&, variable_name] (Value &value) {all_variables[variable_name].SourceUpdate(value);});
                }*/
            }

            /*if (variable->FirstChildElement("transfers")) {
                XMLElement* transfers = variable->FirstChildElement("transfers");

                for (XMLElement* transfer = transfers->FirstChildElement("transfer"); variable != nullptr; transfer = transfer->NextSiblingElement("transfer")) {
                    if (transfer->FirstChildElement("name") && transfer->FirstChildElement("addr") && !transfer->FirstChildElement("node")) {
                        variable_transfer_name = transfer->FirstChildElement("name")->GetText();
                        variable_transfer_addr = transfer->FirstChildElement("addr")->IntText();
                    }
                    if (transfer->FirstChildElement("name") && !transfer->FirstChildElement("addr") && transfer->FirstChildElement("node")) {
                        variable_transfer_name = transfer->FirstChildElement("name")->GetText();
                        variable_transfer_node = transfer->FirstChildElement("node")->GetText();
                    }
                }
            }*/
        }

        //Сортировка по адресу в каждой области памяти (функции)
        for (auto &device : temp_modbus_tcp_client_variables) {
            for (auto &area : device.second) {
                std::sort(area.second.begin(), area.second.end(), [](const auto &a, const auto &b) {return a.addr > b.addr;});
            }
        }

        //Сбор пакетов чтения в каждой области
        for (auto &device : temp_modbus_tcp_client_variables) {
            for (auto &area : device.second) {
                uint16_t addr{0};
                uint16_t len{0};
                uint16_t request_addr{0};
                uint16_t request_len{0};
                uint16_t max_len{0};
                std::vector<ModbusVariable> request_variables;
                for (const auto &variable : area.second) {
                    if (modbus_tcp_client_requests[device.first][area.first].empty()){
                        request_addr = variable.addr;
                        request_len = 1;
                        if (variable.type == "DINT" || variable.type == "UDINT" || variable.type == "DWORD" || variable.type == "REAL") request_len = 2;
                        request_variables.push_back(variable);
                    } else {
                        addr = variable.addr;
                        len = 1;
                        if (variable.type == "DINT" || variable.type == "UDINT" || variable.type == "DWORD" || variable.type == "REAL") len = 2;

                        if (area.first == "holding_registers") {
                            max_len = 125;
                            if (modbus_tcp_client_device_configs[device.first].extended_modbus_tcp) max_len = 719;
                        }

                        if (addr - request_addr + len <= max_len) {
                            request_len = addr - request_addr + len;
                            request_variables.push_back(variable);
                        } else {
                            modbus_tcp_client_requests[device.first][area.first].push_back({request_addr, request_len, request_variables});
                            request_variables.clear();
                            request_addr = variable.addr;
                            request_len = 1;
                            if (variable.type == "DINT" || variable.type == "UDINT" || variable.type == "DWORD" || variable.type == "REAL") request_len = 2;
                            request_variables.push_back(variable);
                        }
                    }
                }
                if (!request_variables.empty()) {
                    modbus_tcp_client_requests[device.first][area.first].push_back({request_addr, request_len, request_variables});
                    request_variables.clear();
                }
            }
        }
    }
}
