#include "modbustcpclient.h"

#include <unistd.h>
#include <cstring>

ModbusTcpClient::ModbusTcpClient(const std::string ip, const int port, const int timeout) : ip_(ip), port_(port), timeout_(timeout), is_connected_(false), should_run_(false) {}

ModbusTcpClient::~ModbusTcpClient() {
    is_connected_ = false;
    should_run_ = false;
    shutdown(socket_, SHUT_RDWR);
    close(socket_);
}

bool ModbusTcpClient::Connect() {
    //Открываем сокет
    socket_ = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_ < 0) {
        IndustrialProtocolUtils::Log("Ошибка открытия сокета \n");
        return is_connected_ = false;
    }
    IndustrialProtocolUtils::Log("Сокет открыт \n");

    //Устанавливаем соединение
    sockaddr_in_.sin_family = AF_INET;
    //sockaddr_in_.sin_addr.s_addr = inet_addr(ip_.c_str());
    sockaddr_in_.sin_port = htons(port_);
    inet_pton(AF_INET, ip_.c_str(), &sockaddr_in_.sin_addr);

    if (connect(socket_, (struct sockaddr*)&sockaddr_in_, sizeof(sockaddr_in_)) < 0) {
        IndustrialProtocolUtils::Log("Ошибка подключения \n");
        shutdown(socket_, SHUT_RDWR);
        close(socket_);
        return is_connected_ = false;
    }
    //std::cout << "Подключение установлено \n";

    // Таймауты на чтение/отправку сообщений
    struct timeval timeout;
    timeout.tv_sec = timeout_ / 1000;  // Установите время ожидания в секундах
    timeout.tv_usec = timeout_ % 1000 * 1000;  // Если необходимо, можно установить микросекунды

    // Установка таймаута для чтения
    if (setsockopt(socket_, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout)) < 0) {
        IndustrialProtocolUtils::Log("Ошибка установки таймаута для чтения \n");
        shutdown(socket_, SHUT_RDWR);
        close(socket_);
        return is_connected_ = false;
    }

    // Установка таймаута для записи
    if (setsockopt(socket_, SOL_SOCKET, SO_SNDTIMEO, (const char*)&timeout, sizeof(timeout)) < 0) {
        IndustrialProtocolUtils::Log("Ошибка установки таймаута для записи \n");
        shutdown(socket_, SHUT_RDWR);
        close(socket_);
        return is_connected_ = false;
    }

    return is_connected_ = true;
}

bool ModbusTcpClient::CheckConnection() {
    return is_connected_;
}

void ModbusTcpClient::Disconnect() {
    if (socket_ > 0) {
        // Отключение сокета
        if (shutdown(socket_, SHUT_RDWR) < 0) {
            IndustrialProtocolUtils::Log("Ошибка отключения сокета \n");
        } else {
            IndustrialProtocolUtils::Log("Сокет корректно отключен \n");
        }
        if (close(socket_) < 0) {
            IndustrialProtocolUtils::Log("Ошибка при закрытии сокета \n");
        } else {
            IndustrialProtocolUtils::Log("Сокет корректно закрыт \n");
        }
    }

    is_connected_ = false;
}

void ModbusTcpClient::WriteHoldingRegisters(const std::vector<std::vector<IndustrialProtocolUtils::DataConfig>>& config_datas, const std::vector<std::vector<uint16_t>>& data) {
    if (is_connected_) {
        if (!config_datas.empty()) {
            for (unsigned long i = 0; i < config_datas.size(); i++) {
                if (is_connected_){
                    int start_address = config_datas[i][0].address;
                    int length = config_datas[i][config_datas[i].size() - 1].address + GetLength(config_datas[i][config_datas[i].size() - 1].type) - config_datas[i][0].address;

                    // Формируем запрос на запись регистров
                    unsigned char request[13 + length * 2];
                    //Идентификатор транзакции
                    request[0] = transaction_id >> 8;
                    request[1] = transaction_id & 0xFF;
                    //Протокол Modbus
                    request[2] = 0x00;
                    request[3] = 0x00;
                    //Длинна пакета
                    request[4] = 0x00;
                    request[5] = 0x07 + length * 2;
                    //Адрес устройства
                    request[6] = 0x00;
                    //Функция
                    request[7] = 0x10;
                    //Начальный адрес
                    request[8] = start_address >> 8;
                    request[9] = start_address & 0xFF;
                    //Количество адресов
                    request[10] = length >> 8;
                    request[11] = length & 0xFF;
                    request[12] = length * 2;

                    for (int j = 0; j < length; j++) {
                        request[j * 2 + 13] = data[i][j] >> 8;
                        request[j * 2 + 14] = data[i][j] & 0xFF;
                    }

                    if (transaction_id >= 32000) {
                        transaction_id = 1;
                    } else {
                        transaction_id += 1;
                    }

                    // Отправляем запрос
                    int request_length = send(socket_, request, sizeof(request), 0);
                    if (request_length < 0) {
                        Disconnect();
                    }
                }
            }
        }
    }
}

void ModbusTcpClient::ReadHoldingRegisters(const std::vector<std::vector<IndustrialProtocolUtils::DataConfig>>& config_datas, std::vector<IndustrialProtocolUtils::DataResult>& data_result) {
    if (is_connected_) {
        if (!config_datas.empty()) {
            data_result.clear();

            for (auto config_data : config_datas) {
                uint16_t tab_reg[125] = {0};
                int start_address = config_data[0].address;
                int length = config_data[config_data.size() - 1].address + GetLength(config_data[config_data.size() - 1].type) - config_data[0].address;

                if (is_connected_){
                    // Формируем запрос на чтение регистров
                    unsigned char request[12];
                    //Идентификатор транзакции
                    request[0] = transaction_id >> 8;
                    request[1] = transaction_id & 0xFF;
                    //Протокол Modbus
                    request[2] = 0x00;
                    request[3] = 0x00;
                    //Длинна пакета
                    request[4] = 0x00;
                    request[5] = 0x06;
                    //Адрес устройства
                    request[6] = 0x00;
                    //Функция
                    request[7] = 0x03;
                    //Начальный адрес
                    request[8] = start_address >> 8;
                    request[9] = start_address & 0xFF;
                    //Количество адресов
                    request[10] = length >> 8;
                    request[11] = length & 0xFF;

                    if (transaction_id >= 32000) {
                        transaction_id = 1;
                    } else {
                        transaction_id += 1;
                    }

                    // Отправляем запрос
                    int request_length = send(socket_, request, sizeof(request), 0);

                    // Получаем ответ
                    if (request_length > 0) {
                        unsigned char response[260] = {0};
                        int response_length = recv(socket_, response, sizeof(response), 0);

                        if (response_length > 0) {
                            if (response[7] == request[7]) {
                                for (int j = 0; j < length; j++) {
                                    tab_reg[j] = (response[j * 2 + 9] << 8) | response[j * 2 + 10];
                                }
                                unsigned int j = 0;
                                while (j < config_data.size()) {
                                    uint16_t data[2];
                                    IndustrialProtocolUtils::DataResult result;
                                    data[0] = tab_reg[config_data[j].address - start_address];
                                    if (GetLength(config_data[j].type) == 2) { data[1] = tab_reg[config_data[j].address - start_address + 1]; }
                                    result.name = config_data[j].name;
                                    result.type = config_data[j].type;
                                    result.quality = true;
                                    result.value = GetValue(result.type, data);
                                    result.address = config_data[j].address;
                                    data_result.push_back(result);
                                    j++;
                                }
                            } else {
                                IndustrialProtocolUtils::Log("Ошибка в ответе: Код функции \n");
                            }
                        } else {
                            IndustrialProtocolUtils::Log("Ошибка получения ответа (превышен интервал ожидания) \n");
                            Disconnect();
                        }
                    } else {
                        Disconnect();
                    }
                }
            }
        }
    }
}

void ModbusTcpClient::ReadHoldingRegistersEx(const std::vector<std::vector<IndustrialProtocolUtils::DataConfig>>& config_datas, std::vector<IndustrialProtocolUtils::DataResult>& data_result) {
    if (is_connected_) {
        if (!config_datas.empty()) {
            data_result.clear();

            for (auto config_data : config_datas) {
                uint16_t tab_reg[125];
                int start_address = config_data[0].address;
                int length = config_data[config_data.size() - 1].address + GetLength(config_data[config_data.size() - 1].type) - config_data[0].address;

                if (is_connected_){
                    // Формируем запрос на чтение регистров
                    unsigned char request[12];
                    //Идентификатор транзакции
                    request[0] = transaction_id >> 8;
                    request[1] = transaction_id & 0xFF;
                    //Протокол Modbus
                    request[2] = 0x00;
                    request[3] = 0x00;
                    //Длинна пакета
                    request[4] = 0x00;
                    request[5] = 0x06;
                    //Адрес устройства
                    request[6] = 0x00;
                    //Функция
                    request[7] = 0x03;
                    //Начальный адрес
                    request[8] = start_address >> 8;
                    request[9] = start_address & 0xFF;
                    //Количество адресов
                    request[10] = length >> 8;
                    request[11] = length & 0xFF;

                    if (transaction_id >= 32000) {
                        transaction_id = 1;
                    } else {
                        transaction_id += 1;
                    }

                    // Отправляем запрос
                    int request_length = send(socket_, request, sizeof(request), 0);

                    // Получаем ответ
                    if (request_length > 0) {
                        unsigned char response[1500];
                        int response_length = recv(socket_, response, sizeof(response), 0);

                        if (response_length > 0) {
                            if (response[7] == request[7]) {
                                for (int j = 0; j < (int)config_data.size(); j++) {
                                    tab_reg[j] = (response[j * 2 + 10] << 8) | response[j * 2 + 9];
                                }
                                unsigned int j = 0;
                                while (j < config_data.size()) {
                                    uint16_t data[2];
                                    IndustrialProtocolUtils::DataResult result;
                                    data[0] = tab_reg[config_data[j].address - start_address];
                                    if (GetLength(config_data[j].type) == 2) { data[1] = tab_reg[config_data[j].address - start_address + 1]; }
                                    result.name = config_data[j].name;
                                    result.type = config_data[j].type;
                                    result.quality = true;
                                    result.value = GetValue(result.type, data);
                                    result.address = config_data[j].address;
                                    data_result.push_back(result);
                                    j++;
                                }
                            } else {
                                IndustrialProtocolUtils::Log("Ошибка в ответе: Код функции \n");
                            }
                        } else {
                            IndustrialProtocolUtils::Log("Ошибка получения ответа (превышен интервал ожидания) \n");
                            Disconnect();
                        }
                    } else {
                        Disconnect();
                    }
                }
            }
        }
    }
}

int ModbusTcpClient::GetLength(const IndustrialProtocolUtils::DataType& type) {
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
    case IndustrialProtocolUtils::DataType::DOUBLE:
        return 4;
    default:
        return 0;
    }
}

IndustrialProtocolUtils::Value ModbusTcpClient::GetValue(const IndustrialProtocolUtils::DataType& type, const uint16_t (&data)[2]) {
    switch (type)
    {
    case IndustrialProtocolUtils::DataType::INT:
        return (IndustrialProtocolUtils::Value) {.i = data[0], .u = 0, .f = 0};
    case IndustrialProtocolUtils::DataType::UINT:
    case IndustrialProtocolUtils::DataType::WORD:
        return (IndustrialProtocolUtils::Value) {.i = 0, .u = data[0], .f = 0};
    case IndustrialProtocolUtils::DataType::DINT:
        return (IndustrialProtocolUtils::Value) {.i = (data[1] << 16) + data[0], .u = 0, .f = 0};
    case IndustrialProtocolUtils::DataType::UDINT:
    case IndustrialProtocolUtils::DataType::DWORD:
        return (IndustrialProtocolUtils::Value) {.i = 0, .u = (uint)(data[1] << 16) + data[0], .f = 0};
    case IndustrialProtocolUtils::DataType::REAL:
        uint8_t bytes[4];

        // Заполняем массив значениями
        bytes[0] = data[0] & 0xFF;          // младший байт low
        bytes[1] = (data[0] >> 8) & 0xFF;   // старший байт low
        bytes[2] = data[1]  & 0xFF;         // младший байт high
        bytes[3] = (data[1]  >> 8) & 0xFF;  // старший байт high

        // Преобразуем массив байтов в float
        float result;
        std::memcpy(&result, bytes, sizeof(result));

        return (IndustrialProtocolUtils::Value) {.i = 0, .u = 0, .f = result};
    default:
        return (IndustrialProtocolUtils::Value) {.i = 0, .u = 0, .f = 0};
    }
}

void ModbusTcpClient::Stop() {
    should_run_ = false;
}
