#include <thread>

#include "industrialprotocolutils.h"
#include "modbustcpclient.h"
#include "opcuaclient.h"
#include "rtm64var.h"

void ThreadModbusTcpClientRead(std::shared_ptr<ModbusTcpClient> modbus_tcp_client, std::vector<std::vector<IndustrialProtocolUtils::DataConfig>>& config_datas, std::vector<IndustrialProtocolUtils::DataResult>& data_result)
{
    modbus_tcp_client->ReadHoldingRegisters(config_datas, data_result);
}

void ThreadModbusTcpClientWrite(std::shared_ptr<ModbusTcpClient> modbus_tcp_client, const std::vector<std::vector<IndustrialProtocolUtils::DataConfig>>& config_datas, const std::vector<std::vector<uint16_t>>& data)
{
    modbus_tcp_client->WriteHoldingRegisters(config_datas, data);
}

void ModbusTcpToOpcUa(const IndustrialProtocolUtils::ModbusTcpDeviceConfig &modbus_tcp_device_config,
                      const std::vector<IndustrialProtocolUtils::DataConfig> &modbus_tcp_to_opc_configs,
                      std::vector<std::shared_ptr<ModbusTcpClient>> modbus_tcp_clients,
                      OpcUaClient &opc_ua_client) {

    //Собираем опрос в группы по 125 регистров
    //std::cout << "//Собираем опрос в группы по 125 регистров" << std::endl;
    std::vector<IndustrialProtocolUtils::DataConfig> configs;
    std::vector<std::vector<IndustrialProtocolUtils::DataConfig>> modbus_configs;
    std::vector<std::vector<IndustrialProtocolUtils::DataResult>> data_results(modbus_tcp_device_config.max_socket_in_eth);

    uint start_address = modbus_tcp_to_opc_configs[0].address;
    for (unsigned int i = 0; i < modbus_tcp_to_opc_configs.size(); i++) {
        if (configs.empty()) {
            configs.push_back(modbus_tcp_to_opc_configs[i]);
        } else {
            uint length = modbus_tcp_to_opc_configs[i].address - start_address + ModbusTcpClient::GetLength(modbus_tcp_to_opc_configs[i].type);
            bool new_thread = false;
            if (!modbus_tcp_device_config.mapping_full_allow) {
                new_thread = modbus_tcp_to_opc_configs[i].address > modbus_tcp_to_opc_configs[i - 1].address + ModbusTcpClient::GetLength(modbus_tcp_to_opc_configs[i - 1].type);
            }

            if (length > 125 || new_thread) {
                modbus_configs.push_back(configs);
                configs.clear();
                start_address = modbus_tcp_to_opc_configs[i].address;
            }
            configs.push_back(modbus_tcp_to_opc_configs[i]);
        }
    }
    modbus_configs.push_back(configs);

    //std::cout << "Всего скомпанованных бочек соединений - " << modbus_configs.size() << std::endl;

    //Собираем группы в потоки по числу максимального количества соединений
    //std::cout << "//Собираем группы в потоки по числу максимального количества соединений" << std::endl;
    std::vector<std::vector<std::vector<IndustrialProtocolUtils::DataConfig>>> tread_modbus_tcp_to_opc_configs(modbus_tcp_device_config.max_socket_in_eth);

    uint max_thread_in_eth = modbus_configs.size() / modbus_tcp_device_config.max_socket_in_eth + modbus_configs.size() % modbus_tcp_device_config.max_socket_in_eth;

    //std::cout << "Всего бочек в каждом соединении - " << max_thread_in_eth << std::endl;

    for (uint i = 0; i < modbus_tcp_device_config.max_socket_in_eth; i++) {
        for (uint j = i * max_thread_in_eth; j < i * max_thread_in_eth + max_thread_in_eth; j++) {
            if (j >= modbus_configs.size()) { break; }
            tread_modbus_tcp_to_opc_configs[i].push_back(modbus_configs[j]);
        }
    }

    //Опрос всех соединений
    //std::cout << "//Опрос всех соединений" << std::endl;
    std::vector<std::thread> threads;
    for (unsigned long i = 0; i < modbus_tcp_clients.size(); i++) {
        if (!(tread_modbus_tcp_to_opc_configs.empty() || data_results.empty())) {
            threads.emplace_back([&, i] () {ThreadModbusTcpClientRead(modbus_tcp_clients[i], tread_modbus_tcp_to_opc_configs[i], data_results[i]);});
        }
    }
    for (auto& th : threads) {
        if (th.joinable()) {
            th.join();
        }
    }

    //Подготовка и запись данных для записи в OPC сервер
    //std::cout << "//Подготовка и запись данных для записи в OPC сервер" << std::endl;
    std::vector<IndustrialProtocolUtils::DataResult> datas;
    for (unsigned long i = 0; i < modbus_tcp_clients.size(); i++) {
        for (unsigned long j = 0; j < data_results[i].size(); j++) {
            datas.push_back(data_results[i][j]);
        }
    }

    opc_ua_client.WriteDatas(datas);
}

void OpcUaToModbusTcp(const IndustrialProtocolUtils::OpcUaDeviceConfig &opc_ua_device_config,
                      const std::vector<IndustrialProtocolUtils::DataConfig> &opc_to_modbus_tcp_configs,
                      std::vector<IndustrialProtocolUtils::DataResult>& data_results,
                      OpcUaClient &opc_ua_client,
                      const IndustrialProtocolUtils::ModbusTcpDeviceConfig &modbus_tcp_device_config,
                      std::vector<std::shared_ptr<ModbusTcpClient>> modbus_tcp_clients) {

    //Считывание данных с OPC сервера
    //std::cout << "//Считывание данных с OPC сервера" << std::endl;
    opc_ua_client.ReadDatas(opc_to_modbus_tcp_configs, data_results);

    //Собираем опрос в группы по 100 регистров
    //std::cout << "//Собираем опрос в группы по 100 регистров" << std::endl;
    std::vector<std::vector<IndustrialProtocolUtils::DataConfig>> thread_modbus_configs;
    std::vector<std::vector<uint16_t>> thread_datas;

    std::vector<IndustrialProtocolUtils::DataConfig> modbus_configs;
    std::vector<uint16_t> datas;

    uint start_address = data_results[0].address;
    for (unsigned int i = 0; i < data_results.size(); i ++) {
        if (data_results[i].time_previos == 0) { data_results[i].time_previos = data_results[i].time_current; }
        if (data_results[i].time_current != data_results[i].time_previos) {
            data_results[i].time_previos = data_results[i].time_current;
            if (modbus_configs.empty()) {
                modbus_configs.push_back({ .address = data_results[i].address, .type = data_results[i].type, .name = data_results[i].name });

                switch (data_results[i].type)
                {
                case IndustrialProtocolUtils::DataType::INT:
                    datas.push_back(data_results[i].value.i);
                case IndustrialProtocolUtils::DataType::UINT:
                case IndustrialProtocolUtils::DataType::WORD:
                    datas.push_back(data_results[i].value.u);
                case IndustrialProtocolUtils::DataType::DINT:
                    datas.push_back(data_results[i].value.i >> 16);
                    datas.push_back(data_results[i].value.i & 65535);
                case IndustrialProtocolUtils::DataType::UDINT:
                case IndustrialProtocolUtils::DataType::DWORD:
                    datas.push_back(data_results[i].value.u >> 16);
                    datas.push_back(data_results[i].value.u & 65535);
                case IndustrialProtocolUtils::DataType::REAL:
                    uint16_t temp[2];
                    memcpy(temp, &data_results[i].value.f, sizeof(float));
                    datas.push_back(temp[0]);
                    datas.push_back(temp[1]);
                }
            } else {
                uint length = modbus_configs[modbus_configs.size() - 1].address - start_address + ModbusTcpClient::GetLength(data_results[i].type);
                bool new_thread = modbus_configs[i].address > modbus_configs[i - 1].address + ModbusTcpClient::GetLength(data_results[i - 1].type);

                if (length > 100 || new_thread) {
                    thread_modbus_configs.push_back(modbus_configs);
                    thread_datas.push_back(datas);
                    modbus_configs.clear();
                    datas.clear();
                    start_address = modbus_configs[i].address;
                }

                modbus_configs.push_back({ .address = data_results[i].address, .type = data_results[i].type, .name = data_results[i].name });

                switch (data_results[i].type)
                {
                case IndustrialProtocolUtils::DataType::INT:
                    datas.push_back(data_results[i].value.i);
                case IndustrialProtocolUtils::DataType::UINT:
                case IndustrialProtocolUtils::DataType::WORD:
                    datas.push_back(data_results[i].value.u);
                case IndustrialProtocolUtils::DataType::DINT:
                    datas.push_back(data_results[i].value.i >> 16);
                    datas.push_back(data_results[i].value.i & 65535);
                case IndustrialProtocolUtils::DataType::UDINT:
                case IndustrialProtocolUtils::DataType::DWORD:
                    datas.push_back(data_results[i].value.u >> 16);
                    datas.push_back(data_results[i].value.u & 65535);
                case IndustrialProtocolUtils::DataType::REAL:
                    uint16_t temp[2];
                    memcpy(temp, &data_results[i].value.f, sizeof(float));
                    datas.push_back(temp[0]);
                    datas.push_back(temp[1]);
                }
            }
        }
    }

    if (!modbus_configs.empty()) {
        thread_modbus_configs.push_back(modbus_configs);
        thread_datas.push_back(datas);
    }

    //Собираем группы в потоки по числу максимального количества соединений
    //std::cout << "//Собираем группы в потоки по числу максимального количества соединений" << std::endl;
    std::vector<std::vector<std::vector<IndustrialProtocolUtils::DataConfig>>> thread_modbus_tcp_to_opc_configs(modbus_tcp_device_config.max_socket_in_eth);
    std::vector<std::vector<std::vector<uint16_t>>> threads_datas(modbus_tcp_device_config.max_socket_in_eth);

    uint max_thread_in_eth = thread_modbus_configs.size() / modbus_tcp_device_config.max_socket_in_eth + thread_modbus_configs.size() % modbus_tcp_device_config.max_socket_in_eth;
    for (uint i = 0; i < modbus_tcp_device_config.max_socket_in_eth; i++) {
        for (uint j = i * max_thread_in_eth; j < i * max_thread_in_eth + max_thread_in_eth; j++) {
            if (j >= max_thread_in_eth) { break; }
            thread_modbus_tcp_to_opc_configs[i].push_back(thread_modbus_configs[j]);
            threads_datas[i].push_back(thread_datas[j]);
        }
    }

    std::vector<std::thread> threads;

    for (unsigned long i = 0; i < thread_modbus_tcp_to_opc_configs.size(); i++) {
        if (!thread_modbus_tcp_to_opc_configs[i].empty())
        threads.emplace_back([&,i] () {ThreadModbusTcpClientWrite(modbus_tcp_clients[i], thread_modbus_tcp_to_opc_configs[i], threads_datas[i]);});
    }

    for (auto& th : threads) {
        if (th.joinable()) {
            th.join();
        }
    }
}

int main() {
    IndustrialProtocolUtils::ModbusTcpDeviceConfig modbus_tcp_device_config;
    std::vector<IndustrialProtocolUtils::DataConfig> modbus_tcp_to_opc_configs;
    std::vector<std::shared_ptr<ModbusTcpClient>> modbus_tcp_clients;

    modbus_tcp_device_config.eth_osn_ip_osn = "192.168.201.1";
    modbus_tcp_device_config.eth_osn_ip_rez = "192.168.201.129";
    modbus_tcp_device_config.eth_rez_ip_osn = "192.168.201.3";
    modbus_tcp_device_config.eth_rez_ip_rez = "192.168.201.131";
    modbus_tcp_device_config.port = 502;
    modbus_tcp_device_config.max_socket_in_eth = 4;
    modbus_tcp_device_config.mapping_full_allow = true;

    IndustrialProtocolUtils::OpcUaDeviceConfig opc_ua_device_config;
    std::vector<IndustrialProtocolUtils::DataConfig> opc_to_modbus_tcp_configs;

    opc_ua_device_config.eth_osn_ip_osn = "opc.tcp://192.168.111.132:62544";
    opc_ua_device_config.port = 62544;

    IndustrialProtocolUtils::ReadConfig(modbus_tcp_device_config, modbus_tcp_to_opc_configs, opc_ua_device_config, opc_to_modbus_tcp_configs);

    for (uint i = 0; i < modbus_tcp_device_config.max_socket_in_eth; i++) {
        modbus_tcp_clients.push_back(std::make_shared<ModbusTcpClient>());
    }

    OpcUaClient opc_ua_client(opc_ua_device_config.eth_osn_ip_osn, opc_ua_device_config.port);

    std::vector<IndustrialProtocolUtils::DataResult> opc_to_modbus_results(opc_to_modbus_tcp_configs.size());

    while (true) {
        //Если нет связи с OPC сервером, то нет смысла обрабатывать логику
        if (!opc_ua_client.CheckConnection()) {
            //std::cout << "Нет связи с OPC сервером" << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(5000));
            continue;
        }
        //Если нет связи с Modbus устройством, то нет смысла обрабатывать логику
        if (!modbus_tcp_clients[0]->CheckConnection()) {
            if (!(modbus_tcp_clients[0]->Connect(modbus_tcp_device_config.eth_osn_ip_osn, modbus_tcp_device_config.port))) {
                if (!(modbus_tcp_clients[0]->Connect(modbus_tcp_device_config.eth_osn_ip_rez, modbus_tcp_device_config.port))) {
                    if (!(modbus_tcp_clients[0]->Connect(modbus_tcp_device_config.eth_rez_ip_osn, modbus_tcp_device_config.port))) {
                        if (!(modbus_tcp_clients[0]->Connect(modbus_tcp_device_config.eth_rez_ip_rez, modbus_tcp_device_config.port))) {
                            //std::cout << "Нет связи с Modbus устройстом" << std::endl;
                            std::this_thread::sleep_for(std::chrono::milliseconds(5000));
                            continue;
                        } else {
                            for (uint i = 1; i < modbus_tcp_device_config.max_socket_in_eth; i++) {
                                modbus_tcp_clients[i]->Connect(modbus_tcp_device_config.eth_rez_ip_rez, modbus_tcp_device_config.port);
                            }
                        }
                    } else {
                        for (uint i = 1; i < modbus_tcp_device_config.max_socket_in_eth; i++) {
                            modbus_tcp_clients[i]->Connect(modbus_tcp_device_config.eth_rez_ip_osn, modbus_tcp_device_config.port);
                        }
                    }
                } else {
                    for (uint i = 1; i < modbus_tcp_device_config.max_socket_in_eth; i++) {
                        modbus_tcp_clients[i]->Connect(modbus_tcp_device_config.eth_osn_ip_rez, modbus_tcp_device_config.port);
                    }
                }
            } else {
                for (uint i = 1; i < modbus_tcp_device_config.max_socket_in_eth; i++) {
                    modbus_tcp_clients[i]->Connect(modbus_tcp_device_config.eth_osn_ip_osn, modbus_tcp_device_config.port);
                }
            }
        }

        //std::cout << "Beg OpcUaToModbusTcp" << std::endl;
        OpcUaToModbusTcp(opc_ua_device_config, opc_to_modbus_tcp_configs, opc_to_modbus_results, opc_ua_client, modbus_tcp_device_config, modbus_tcp_clients);
        //std::cout << "End OpcUaToModbusTcp" << std::endl;
        //std::cout << "Beg ModbusTcpToOpcUa" << std::endl;
        ModbusTcpToOpcUa(modbus_tcp_device_config, modbus_tcp_to_opc_configs, modbus_tcp_clients, opc_ua_client);
        //std::cout << "End ModbusTcpToOpcUa" << std::endl;

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    return 0;
}
