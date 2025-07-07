#include <thread>

#include "industrialprotocolutils.h"
#include "modbustcpclient.h"
#include <memory>

void ModbusTcpClientControl(std::vector<std::shared_ptr<ModbusTcpClient>> client,
                            const ModbusTcpClientDeviceConfig &config,
                            Variables &variables,
                            std::map<std::string, std::vector<ModbusRequestConfig>> &requests) {
    if (client[0]->CheckConnection()) {
        if (config.extended_modbus_tcp)
        {
            client[0]->ReadHoldingRegistersEx(requests["HoldingRegisters"][0]);
        }
        else
        {
            client[0]->ReadHoldingRegisters(requests["HoldingRegisters"][0]);
            client[0]->ReadHoldingRegisters(requests["HoldingRegisters"][1]);
            client[0]->ReadHoldingRegisters(requests["HoldingRegisters"][2]);
            client[0]->ReadHoldingRegisters(requests["HoldingRegisters"][3]);
            client[0]->ReadHoldingRegisters(requests["HoldingRegisters"][4]);
            client[0]->ReadHoldingRegisters(requests["HoldingRegisters"][5]);
            client[0]->ReadHoldingRegisters(requests["HoldingRegisters"][6]);
            client[0]->ReadHoldingRegisters(requests["HoldingRegisters"][7]);
        }

        client[0]->ReceiveResponse();
    } else {
        client[0]->Connect();
    }
}

int main() {
    Log("Сервер запущен");

    //Глобальные структуры сервера
    std::map<std::string, Variable> server_variables;

    //Структыр для работы с Modbus устройствами
    std::map<std::string, std::vector<std::shared_ptr<ModbusTcpClient>>> modbus_tcp_clients;
    std::map<std::string, ModbusTcpClientDeviceConfig> modbus_tcp_client_device_configs;
    std::map<std::string, Variables> modbus_tcp_client_variables;
    std::map<std::string, std::map<std::string, std::vector<ModbusRequestConfig>>> modbus_tcp_client_requests;
    std::map<std::string, ModbusMemory> modbus_tcp_client_memory;
    std::map<std::string, std::mutex> modbus_tcp_client_mutex_memory;
    std::map<std::string, std::mutex> modbus_tcp_client_mutex_variables;
    std::map<std::string, std::mutex> modbus_tcp_client_mutex_transaction_id;

    std::map<std::string, OpcUaClientDeviceConfig> opc_ua_client_device_configs;

    Log("Считывание конфигурации");
    ReadConfig(modbus_tcp_client_device_configs,
               opc_ua_client_device_configs,
               server_variables,
               modbus_tcp_client_variables,
               modbus_tcp_client_requests);

    Log("Всего переменных сервера: " + std::to_string(server_variables.size()));

    Log("Всего Modbus устройств: " + std::to_string(modbus_tcp_client_device_configs.size()));
    for (const auto &device : modbus_tcp_client_device_configs) {
        modbus_tcp_clients.emplace(device.first, std::make_shared<ModbusTcpClient>(modbus_tcp_client_device_configs[device.first].addr[0],
                                                                                   modbus_tcp_client_device_configs[device.first].port[0],
                                                                                   modbus_tcp_client_device_configs[device.first].timeout,
                                                                                   modbus_tcp_client_memory[device.first],
                                                                                   modbus_tcp_client_variables[device.first],
                                                                                   modbus_tcp_client_mutex_memory[device.first],
                                                                                   modbus_tcp_client_mutex_variables[device.first],
                                                                                   modbus_tcp_client_mutex_transaction_id[device.first]));
        Log("Устройство: " + device.first);
        Log("Переменных для обработки: " + std::to_string(modbus_tcp_client_variables[device.first].size()));
        Log("Количество запросов: " + std::to_string(modbus_tcp_client_requests[device.first]["HoldingRegisters"].size()));
    }
    Log("Всего PC UA устройств: " + std::to_string(opc_ua_client_device_configs.size()));
    for (const auto &device : opc_ua_client_device_configs) {
        Log("Устройство: " + device.first);
    }

    while (true) {
        for (const auto &device : modbus_tcp_client_device_configs) {
            ModbusTcpClientControl(modbus_tcp_clients[device.first], modbus_tcp_client_device_configs[device.first], modbus_tcp_client_variables[device.first], modbus_tcp_client_requests[device.first]);
        }
    }

    return 0;
}
