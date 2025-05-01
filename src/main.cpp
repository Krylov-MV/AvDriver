#include <thread>
#include <memory>
#include <iostream>
#include <sstream>
#include <fstream>
#include <algorithm>

#include "modbustcpclient.h"
#include "opcuaclient.h"

struct DataLinks {
    std::string node_id;
    int offset;
};

struct BridgeModbusOpc {
    std::map<std::string, int> to_modbus;
    std::map<std::string, int> to_opc;
};

void ThreadModbusTcpClientWriteRead(std::shared_ptr<ModbusTcpClient> client,
                                    const ModbusTcpClient::WriteReadConfigs& write_read_configs,
                                    ModbusTcpClient::Memory& memory,
                                    std::mutex& mutex) {
    //std::cout << "Выполняется чтение/запись данных Modbus устройства в потоке \n";

    client->WriteCoils(write_read_configs.write.coils);
    client->WriteHoldingRegisters(write_read_configs.write.holding_registers);

    client->ReadDiscreteInputs(write_read_configs.read.discrete_inputs, memory.discrete_inputs, mutex);
    client->ReadCoils(write_read_configs.read.coils, memory.coils, mutex);
    client->ReadInputRegisters(write_read_configs.read.input_registers, memory.input_registers, mutex);
    client->ReadHoldingRegisters(write_read_configs.read.holding_registers, memory.holding_registers, mutex);
}

void ModbusTcpClientRun(std::vector<std::shared_ptr<ModbusTcpClient>> clients,
                        const ModbusTcpClient::DeviceConfig& device_config,
                        const ModbusTcpClient::WriteReadConfigs& write_read_configs,
                        ModbusTcpClient::Memory& memory,
                        std::mutex& mutex) {
    ModbusTcpClient::WriteReadConfigs write_read_configs_;
    //Подготовка карты записи Modbus устройства
    if (!write_read_configs.write.coils.empty()) {
        write_read_configs_.write.coils = ModbusTcpClient::PrepareModbusWriteBits(write_read_configs.write.coils, MODBUS_MAX_WRITE_BITS);
    }

    //Подготовка карты записи/опроса Modbus устройства
    //if (!device_config.write_read_registers_allow) {
        if (!write_read_configs.write.holding_registers.empty()) {
            write_read_configs_.write.holding_registers = ModbusTcpClient::PrepareModbusWriteRegisters(write_read_configs.write.holding_registers, MODBUS_MAX_WRITE_REGISTERS);
        }

        if (!write_read_configs.read.input_registers.empty()) {
            write_read_configs_.read.input_registers = ModbusTcpClient::PrepareModbusRead(write_read_configs.read.input_registers, MODBUS_MAX_READ_REGISTERS);
        }
        if (!write_read_configs.read.holding_registers.empty()) {
            write_read_configs_.read.holding_registers = ModbusTcpClient::PrepareModbusRead(write_read_configs.read.holding_registers, MODBUS_MAX_READ_REGISTERS);
        }
    //} else {
    //}

    //Если карта записи/опроса пуста, то завершаем работу
    if (write_read_configs_.read.discrete_inputs.empty() && write_read_configs_.read.coils.empty() &&
        write_read_configs_.read.input_registers.empty() && write_read_configs_.read.holding_registers.empty() &&
        write_read_configs_.write.coils.empty() && write_read_configs_.write.holding_registers.empty()) { return; }

    //Определение доступных сокетов
    int socket_count = 0;
    for (int i = 0; i < clients.size(); i++) {
        if (clients[i]->CheckConnection()) {
            socket_count++;
        }
    }

    //std::cout << "Количество сокетов для чтения/записи - " << socket_count << "\n";

    //Определение количества запросов чтения/запиши на каждый сокет
    int write_read_in_socket_count = write_read_configs_.read.discrete_inputs.size() +
        write_read_configs_.read.coils.size() + write_read_configs_.read.input_registers.size() +
        write_read_configs_.read.holding_registers.size() +
        write_read_configs_.write.coils.size() + write_read_configs_.write.holding_registers.size();

    if (write_read_in_socket_count % socket_count == 0) {
        write_read_in_socket_count = write_read_in_socket_count / socket_count;
    } else {
        write_read_in_socket_count = write_read_in_socket_count / socket_count + 1;
    }

    //std::cout << "Количество бочек для чтения/записи на каждый сокет - " << write_read_in_socket_count << "\n";

    //Подготоваливаем карту чтения/записи по количеству запросов чтения/записи на каждый сокет
    std::vector<ModbusTcpClient::WriteReadConfigs> thread_write_read_configs(socket_count);

    int num_socket = 0;
    int num_write_read_in_socket = 0;

    if(!write_read_configs_.write.coils.empty()) {
        for (int i = 0; i < write_read_configs_.write.coils.size(); i++) {
            thread_write_read_configs[num_socket].write.coils.push_back(write_read_configs_.write.coils[i]);
            num_write_read_in_socket++;
            if (thread_write_read_configs[num_socket].write.coils.size() >= write_read_in_socket_count) {
                num_socket++;
                num_write_read_in_socket = 0;
            }
        }
    }

    if(!write_read_configs_.write.coils.empty()) {
    for (int i = 0; i < write_read_configs_.write.holding_registers.size(); i++) {
        thread_write_read_configs[num_socket].write.holding_registers.push_back(write_read_configs_.write.holding_registers[i]);
        num_write_read_in_socket++;
        if (thread_write_read_configs[num_socket].write.holding_registers.size() >= write_read_in_socket_count) {
            num_socket++;
            num_write_read_in_socket = 0;
        }
    }
    }
    if(!write_read_configs_.read.discrete_inputs.empty()) {
        for (int i = 0; i < write_read_configs_.read.discrete_inputs.size(); i++) {
            thread_write_read_configs[num_socket].read.discrete_inputs.push_back(write_read_configs_.read.discrete_inputs[i]);
            num_write_read_in_socket++;
            if (thread_write_read_configs[num_socket].read.discrete_inputs.size() >= write_read_in_socket_count) {
                num_socket++;
                num_write_read_in_socket = 0;
            }
        }
    }
    if(!write_read_configs_.read.coils.empty()) {
        for (int i = 0; i < write_read_configs_.read.coils.size(); i++) {
            thread_write_read_configs[num_socket].read.coils.push_back(write_read_configs_.read.coils[i]);
            num_write_read_in_socket++;
            if (thread_write_read_configs[num_socket].read.coils.size() > write_read_in_socket_count) {
                num_socket++;
                num_write_read_in_socket = 0;
            }
        }
    }
    if(!write_read_configs_.read.input_registers.empty()) {
        for (int i = 0; i < write_read_configs_.read.input_registers.size(); i++) {
            thread_write_read_configs[num_socket].read.input_registers.push_back(write_read_configs_.read.input_registers[i]);
            num_write_read_in_socket++;
            if (thread_write_read_configs[num_socket].read.input_registers.size() >= write_read_in_socket_count) {
                num_socket++;
                num_write_read_in_socket = 0;
            }
        }
    }
    if(!write_read_configs_.read.holding_registers.empty()) {
        for (int i = 0; i < write_read_configs_.read.holding_registers.size(); i++) {
            thread_write_read_configs[num_socket].read.holding_registers.push_back(write_read_configs_.read.holding_registers[i]);
            num_write_read_in_socket++;
            if (thread_write_read_configs[num_socket].read.holding_registers.size() >= write_read_in_socket_count) {
                num_socket++;
                num_write_read_in_socket = 0;
            }
        }
    }

    /*for (ModbusTcpClient::ReadConfig& holding_registers : thread_write_read_configs[0].read.holding_registers) {
        std::cout << holding_registers.offset << std::endl;
        std::cout << holding_registers.length << std::endl;
    }*/

    //Опрос всех соединений
    std::vector<std::thread> threads;
    for (int i = 0; i < socket_count; i++) {
        threads.emplace_back([&, i] () {ThreadModbusTcpClientWriteRead(clients[i], thread_write_read_configs[i], memory, mutex);});
    }
    for (auto& th : threads) {
        if (th.joinable()) {
            th.join();
        }
    }
}

void OpcUaClientRun(OpcUaClient& client,
                    const OpcUaClient::DeviceConfig& device_config,
                    OpcUaClient::WriteReadConfigs& write_read_configs,
                    std::map<std::string, OpcUaClient::OpcUaValue>& read_results) {
    if (!write_read_configs.write.empty()) { client.Write(write_read_configs.write); }

    if (!write_read_configs.read.empty()) { client.Read(write_read_configs.read, read_results); }
}

void ReadConfig_0_1(ModbusTcpClient::DeviceConfig& modbus_tcp_client_device_config,
                    ModbusTcpClient::WriteReadConfigs& modbus_tcp_client_write_read_configs,
                    OpcUaClient::DeviceConfig& opc_ua_client_device_config,
                    OpcUaClient::WriteReadConfigs& opc_ua_client_write_read_configs,
                    BridgeModbusOpc& bridge_modbus_opc) {
    std::string line;

    std::ifstream file("devices.txt");
    getline(file, line);
    std::istringstream(line) >> modbus_tcp_client_device_config.max_socket_in_eth >> modbus_tcp_client_device_config.port
                             >> modbus_tcp_client_device_config.eth_osn_ip_osn >> modbus_tcp_client_device_config.eth_osn_ip_rez
                             >> modbus_tcp_client_device_config.eth_rez_ip_osn >> modbus_tcp_client_device_config.eth_rez_ip_rez;
    getline(file, line);
    std::istringstream(line) >> opc_ua_client_device_config.url;

    file.close();

    file.open("configs.txt");
    while (getline(file, line))
    {
        std::string from_device{""};
        std::string from_function{""};
        std::string from_address{""};
        std::string from_type{""};
        std::string from_length{""};

        std::string to_device{""};
        std::string to_function{""};
        std::string to_address{""};
        std::string to_type{""};
        std::string to_length{""};

        std::istringstream(line) >> (from_device) >> (from_function) >> (from_address) >> (from_type) >> (from_length)
                                 >> (to_device)   >> (to_function)   >> (to_address)   >> (to_type)   >> (to_length);

        if (ModbusTcpClient::TypeLength(from_type) > 0 && (std::stoi(from_address) >= 0 && std::stoi(from_address) <= 65535) && OpcUaClient::TypeLength(to_type) > 0 && !to_address.empty()) {
            if(from_function == "3") {
                modbus_tcp_client_write_read_configs.read.holding_registers.push_back({std::stoi(from_address), ModbusTcpClient::TypeLength(from_type)});
                opc_ua_client_write_read_configs.write.push_back({false, to_address, to_type});
                bridge_modbus_opc.to_opc[to_address] = std::stoi(from_address);
            }
            if(from_function == "16") {
                modbus_tcp_client_write_read_configs.write.holding_registers.push_back({std::stoi(from_address), ModbusTcpClient::TypeLength(from_type)});
                opc_ua_client_write_read_configs.read.push_back({to_address, to_type});
                bridge_modbus_opc.to_modbus[to_address] = std::stoi(from_address);
            }
            if(from_function == "23") {
                modbus_tcp_client_write_read_configs.write.holding_registers.push_back({std::stoi(from_address), ModbusTcpClient::TypeLength(from_type)});
                modbus_tcp_client_write_read_configs.read.holding_registers.push_back({std::stoi(from_address), ModbusTcpClient::TypeLength(from_type)});
                opc_ua_client_write_read_configs.write.push_back({false, to_address, to_type});
                opc_ua_client_write_read_configs.read.push_back({to_address + ".write", to_type});

                bridge_modbus_opc.to_opc[to_address] = std::stoi(from_address);
                bridge_modbus_opc.to_modbus[to_address + ".write"] = std::stoi(from_address);
            }
        }
    }
    file.close();

    std::sort(modbus_tcp_client_write_read_configs.write.coils.begin(), modbus_tcp_client_write_read_configs.write.coils.end(),
        [](const ModbusTcpClient::WriteBitsConfigs& a, const ModbusTcpClient::WriteBitsConfigs& b) { return a.offset < b.offset; });
    std::sort(modbus_tcp_client_write_read_configs.write.holding_registers.begin(), modbus_tcp_client_write_read_configs.write.holding_registers.end(),
        [](const ModbusTcpClient::WriteRegistersConfig& a, const ModbusTcpClient::WriteRegistersConfig& b) { return a.offset < b.offset; });

    std::sort(modbus_tcp_client_write_read_configs.read.discrete_inputs.begin(), modbus_tcp_client_write_read_configs.read.discrete_inputs.end(),
        [](const ModbusTcpClient::ReadConfig& a, const ModbusTcpClient::ReadConfig& b) { return a.offset < b.offset; });
    std::sort(modbus_tcp_client_write_read_configs.read.coils.begin(), modbus_tcp_client_write_read_configs.read.coils.end(),
        [](const ModbusTcpClient::ReadConfig& a, const ModbusTcpClient::ReadConfig& b) { return a.offset < b.offset; });
    std::sort(modbus_tcp_client_write_read_configs.read.input_registers.begin(), modbus_tcp_client_write_read_configs.read.input_registers.end(),
        [](const ModbusTcpClient::ReadConfig& a, const ModbusTcpClient::ReadConfig& b) { return a.offset < b.offset; });
    std::sort(modbus_tcp_client_write_read_configs.read.holding_registers.begin(), modbus_tcp_client_write_read_configs.read.holding_registers.end(),
        [](const ModbusTcpClient::ReadConfig& a, const ModbusTcpClient::ReadConfig& b) { return a.offset < b.offset; });
}

int main() {
    //Инициализация/объявления
    BridgeModbusOpc bridge_modbus_opc;
    ModbusTcpClient::DeviceConfig modbus_tcp_client_device_config;
    ModbusTcpClient::WriteReadConfigs modbus_tcp_client_write_read_configs;
    OpcUaClient::DeviceConfig opc_ua_client_device_config;
    OpcUaClient::WriteReadConfigs opc_ua_client_write_read_configs;

    //Считывание конфигурации
    std::cout << "Считывание конфигурации \n";
    ReadConfig_0_1(modbus_tcp_client_device_config,
                   modbus_tcp_client_write_read_configs,
                   opc_ua_client_device_config,
                   opc_ua_client_write_read_configs,
                   bridge_modbus_opc);
    std::cout << "Конфигурация считана \n";

    ModbusTcpClient::Memory modbus_tcp_client_memory;
    std::map<std::string, OpcUaClient::OpcUaValue> opc_ua_client_read_results;

    std::vector<std::shared_ptr<ModbusTcpClient>> modbus_tcp_client_clients;
    std::mutex modbus_tcp_client_mutex;
    for (uint i = 0; i < modbus_tcp_client_device_config.max_socket_in_eth * 4; i++) {
        modbus_tcp_client_clients.push_back(std::make_shared<ModbusTcpClient>());
    }

    OpcUaClient opc_ua_client;
    opc_ua_client.SetUrl(opc_ua_client_device_config.url);

    //Логика программы
    std::cout << "Начало выполнения основной части программы \n";
    while (true) {
        //Если нет связи с OPC сервером, то нет смысла обрабатывать логику
        if (!opc_ua_client.CheckConnection()) {
            std::cout << "Нет связи с OPC сервером \n";
            std::this_thread::sleep_for(std::chrono::milliseconds(5000));
            continue;
        }
        //Если нет связи с Modbus устройством, то нет смысла обрабатывать логику
        bool link_is_fail = true;
        uint j = 0;
        if (!modbus_tcp_client_clients[j]->CheckConnection()) {
            if (modbus_tcp_client_clients[j]->Connect(modbus_tcp_client_device_config.eth_osn_ip_osn, modbus_tcp_client_device_config.port)) {
                j++;
                for (uint i = 1; i < modbus_tcp_client_device_config.max_socket_in_eth; i++) {
                    if (modbus_tcp_client_clients[j]->Connect(modbus_tcp_client_device_config.eth_osn_ip_osn, modbus_tcp_client_device_config.port)) { j++; }
                }
            }
        }
        if (!modbus_tcp_client_device_config.eth_osn_ip_rez.empty()){
            if (!modbus_tcp_client_clients[j]->CheckConnection()) {
                if (modbus_tcp_client_clients[j]->Connect(modbus_tcp_client_device_config.eth_osn_ip_rez, modbus_tcp_client_device_config.port)) {
                    j++;
                    for (uint i = 1; i < modbus_tcp_client_device_config.max_socket_in_eth; i++) {
                        if (modbus_tcp_client_clients[j]->Connect(modbus_tcp_client_device_config.eth_osn_ip_rez, modbus_tcp_client_device_config.port)) { j++; }
                    }
                }
            }
        }
        if (!modbus_tcp_client_device_config.eth_rez_ip_osn.empty()){
            if (!modbus_tcp_client_clients[j]->CheckConnection()) {
                if (modbus_tcp_client_clients[j]->Connect(modbus_tcp_client_device_config.eth_rez_ip_osn, modbus_tcp_client_device_config.port)) {
                    j++;
                    for (uint i = 1; i < modbus_tcp_client_device_config.max_socket_in_eth; i++) {
                        if (modbus_tcp_client_clients[j]->Connect(modbus_tcp_client_device_config.eth_rez_ip_osn, modbus_tcp_client_device_config.port)) { j++; }
                    }
                }
            }
        }
        if (!modbus_tcp_client_device_config.eth_rez_ip_rez.empty()){
            if (!modbus_tcp_client_clients[j]->CheckConnection()) {
                if (modbus_tcp_client_clients[j]->Connect(modbus_tcp_client_device_config.eth_rez_ip_rez, modbus_tcp_client_device_config.port)) {
                    j++;
                    for (uint i = 1; i < modbus_tcp_client_device_config.max_socket_in_eth; i++) {
                        if (modbus_tcp_client_clients[j]->Connect(modbus_tcp_client_device_config.eth_rez_ip_rez, modbus_tcp_client_device_config.port)) { j++; }
                    }
                }
            }
        }

        for (unsigned int i = 0; i < modbus_tcp_client_clients.size(); i++) {
            if (modbus_tcp_client_clients[i]->CheckConnection()) {
                link_is_fail = false;
                break;
            }
        }

        if (link_is_fail) {
            std::cout << "Нет связи с Modbus устройством \n";
            std::this_thread::sleep_for(std::chrono::milliseconds(5000));
            continue;
        }

        //std::cout << "Выполняется чтение/запись Modbus \n";
        //Подготовка данных для передачи в Modbus
        ModbusTcpClientRun(modbus_tcp_client_clients,
                           modbus_tcp_client_device_config,
                           modbus_tcp_client_write_read_configs,
                           modbus_tcp_client_memory,
                           modbus_tcp_client_mutex);

        //Подготовка данных для передачи в OPC
        for (const std::pair<std::string, int>& bridge : bridge_modbus_opc.to_opc) {
            ;
        }
        OpcUaClientRun(opc_ua_client,
                       opc_ua_client_device_config,
                       opc_ua_client_write_read_configs,
                       opc_ua_client_read_results);

        //std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    return 0;
}
