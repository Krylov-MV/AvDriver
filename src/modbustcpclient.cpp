#include "modbustcpclient.h"

#include <unistd.h>
#include <cstring>

ModbusTcpClient::ModbusTcpClient(const std::string ip, const int port, const int timeout, ModbusMemory &memory, std::map<std::string, Variable> &variables, std::mutex &mutex_memory, std::mutex &mutex_variables) :
    ip_(ip), port_(port), timeout_(timeout), memory_(memory), variables_(variables), mutex_memory_(mutex_memory), mutex_variables_(mutex_variables), is_connected_(false) {}

ModbusTcpClient::~ModbusTcpClient() {
    is_connected_ = false;
    shutdown(socket_, SHUT_RDWR);
    close(socket_);
}

bool ModbusTcpClient::Connect() {
    //Открываем сокет
    socket_ = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_ < 0) {
        Log("Ошибка открытия сокета \n");
        return is_connected_ = false;
    }
    Log("Сокет открыт \n");

    //Устанавливаем соединение
    sockaddr_in_.sin_family = AF_INET;
    //sockaddr_in_.sin_addr.s_addr = inet_addr(ip_.c_str());
    sockaddr_in_.sin_port = htons(port_);
    inet_pton(AF_INET, ip_.c_str(), &sockaddr_in_.sin_addr);

    if (connect(socket_, (struct sockaddr*)&sockaddr_in_, sizeof(sockaddr_in_)) < 0) {
        Log("Ошибка подключения \n");
        shutdown(socket_, SHUT_RDWR);
        close(socket_);
        return is_connected_ = false;
    }
    Log("Подключение установлено \n");

    // Таймауты на чтение/отправку сообщений
    struct timeval timeout;
    timeout.tv_sec = timeout_ / 1000;  // Установите время ожидания в секундах
    timeout.tv_usec = timeout_ % 1000 * 1000;  // Если необходимо, можно установить микросекунды

    // Установка таймаута для чтения
    if (setsockopt(socket_, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout)) < 0) {
        Log("Ошибка установки таймаута для чтения \n");
        shutdown(socket_, SHUT_RDWR);
        close(socket_);
        return is_connected_ = false;
    }

    // Установка таймаута для записи
    if (setsockopt(socket_, SOL_SOCKET, SO_SNDTIMEO, (const char*)&timeout, sizeof(timeout)) < 0) {
        Log("Ошибка установки таймаута для записи \n");
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
            Log("Ошибка отключения сокета \n");
        } else {
            Log("Сокет корректно отключен \n");
        }
        if (close(socket_) < 0) {
            Log("Ошибка при закрытии сокета \n");
        } else {
            Log("Сокет корректно закрыт \n");
        }
    }

    is_connected_ = false;
}

void ModbusTcpClient::IncTransactionId() {
    if (transaction_id_ >= 255) {
        transaction_id_ = 1;
    } else {
        transaction_id_ += 1;
    }
}

int ModbusTcpClient::SendRequest(const uint8_t *request) {
    return send(socket_, request, 0x06 + request[5], 0);
}

int ModbusTcpClient::ReceiveResponse() {
    while (!queue_.empty()) {
        unsigned char response[1500] = {0};

        int response_length = recv(socket_, response, sizeof(response), 0);

        if (response_length > 0) {
            uint64_t unix_timestamp_ms = static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count());
            uint8_t function{0};
            uint16_t addr{0};
            uint16_t len{0};
            std::vector<ModbusVariable> variables;
            {
                std::lock_guard<std::mutex> lock(mutex_transaction_id_);
                const auto it = queue_.find(response[1]);
                if (it != queue_.end()) {
                    function = it->second.function;
                    addr = it->second.addr_high_byte << 8 | it->second.addr_low_byte;
                    len = it->second.len_high_byte << 8 | it->second.len_low_byte;
                    variables = it->second.variables;
                    queue_.erase(it);
                } else {
                    //Если пакет в очереди не найден, значит прилетел мусор
                    continue;
                }
            }

            if ((response[7] == function) && (len & 0xFF) == response[8]) {
                if (function == 0x03 || function == 0x43) {
                    for (int j = 0; j < len; j++) {
                        uint16_t value{0};
                        if (function == 0x03) value = (response[j * 2 + 9] << 8) | response[j * 2 + 10];
                        if (function == 0x43) value = (response[j * 2 + 10] << 8) | response[j * 2 + 9];
                        std::lock_guard<std::mutex> lock(mutex_memory_);
                        memory_.holding_registers[addr + j] = {value, 0, unix_timestamp_ms};
                    }
                    for (const ModbusVariable &variable : variables) {
                        GetValue(variable);
                    }
                }
            }
        } else {
            Log("Ошибка получения ответа (превышен интервал ожидания) \n");
            Disconnect();
            break;
        }
    }
}

void ModbusTcpClient::WriteHoldingRegisters(const ModbusRequestConfig &config, const uint16_t *value) {
    if (is_connected_) {
        // Формируем запрос на запись регистров
        unsigned char request[13 + config.len * 2];
        //Идентификатор транзакции
        request[0] = transaction_id_ >> 8;
        request[1] = transaction_id_ & 0xFF;
        //Протокол Modbus
        request[2] = 0x00;
        request[3] = 0x00;
        //Длинна пакета
        request[4] = 0x00;
        request[5] = 0x07 + config.len * 2;
        //Адрес устройства
        request[6] = 0x01;
        //Функция
        request[7] = 0x10;
        //Начальный адрес
        request[8] = config.addr >> 8;
        request[9] = config.addr & 0xFF;
        //Количество адресов
        request[10] = config.len >> 8;
        request[11] = config.len & 0xFF;
        request[12] = config.len * 2;

        for (int i = 0; i < config.len; i++) {
            request[i * 2 + 13] = value[i] >> 8;
            request[i * 2 + 14] = value[i] & 0xFF;
        }

        //Добавляем пакет в очередь ожидания ответа
        {
            queue_[transaction_id_] = {request[7], request[8], request[9], request[10], request[11], {}};
            std::lock_guard<std::mutex> lock(mutex_transaction_id_);
            IncTransactionId();
        }

        //Отправляем пакет данных
        int request_length = SendRequest(request);

        //Если отправка не удалась отключаемся и очищаем очередь
        if (request_length < 0) {
            std::lock_guard<std::mutex> lock(mutex_transaction_id_);
            queue_.clear();
            Disconnect();
        }
    }
}

void ModbusTcpClient::ReadHoldingRegisters(const ModbusRequestConfig &config) {
    if (is_connected_) {
        // Формируем запрос на чтение регистров
        uint8_t request[12];
        //Идентификатор транзакции
        request[0] = 0x00;
        request[1] = transaction_id_;
        //Протокол Modbus
        request[2] = 0x00;
        request[3] = 0x00;
        //Длинна пакета
        request[4] = 0x00;
        request[5] = 0x06;
        //Адрес устройства
        request[6] = 0x01;
        //Функция
        request[7] = 0x03;
        //Начальный адрес
        request[8] = config.addr >> 8;
        request[9] = config.addr & 0xFF;
        //Количество адресов
        request[10] = config.len >> 8;
        request[11] = config.len & 0xFF;

        //Добавляем пакет в очередь ожидания ответа
        {
            queue_[transaction_id_] = {request[7], request[8], request[9], request[10], request[11], config.variables};
            std::lock_guard<std::mutex> lock(mutex_transaction_id_);
            IncTransactionId();
        }

        //Отправляем пакет данных
        int request_length = SendRequest(request);

        //Если отправка не удалась отключаемся и очищаем очередь
        if (request_length < 0) {
            std::lock_guard<std::mutex> lock(mutex_transaction_id_);
            queue_.clear();
            Disconnect();
        }
    }
}

void ModbusTcpClient::ReadHoldingRegistersEx(const ModbusRequestConfig &config) {
    if (is_connected_) {
        // Формируем запрос на чтение регистров
        uint8_t request[12];
        //Идентификатор транзакции
        request[0] = 0x00;
        request[1] = transaction_id_;
        //Протокол Modbus
        request[2] = 0x00;
        request[3] = 0x00;
        //Длинна пакета
        request[4] = 0x00;
        request[5] = 0x06;
        //Адрес устройства
        request[6] = 0x01;
        //Функция
        request[7] = 0x43;
        //Начальный адрес
        request[8] = config.addr >> 8;
        request[9] = config.addr & 0xFF;
        //Количество адресов
        request[10] = config.len >> 8;
        request[11] = config.len & 0xFF;

        //Добавляем пакет в очередь ожидания ответа
        {
            queue_[transaction_id_] = {request[7], request[8], request[9], request[10], request[11], config.variables};
            std::lock_guard<std::mutex> lock(mutex_transaction_id_);
            IncTransactionId();
        }

        //Отправляем пакет данных
        int request_length = SendRequest(request);

        //Если отправка не удалась отключаемся и очищаем очередь
        if (request_length < 0) {
            std::lock_guard<std::mutex> lock(mutex_transaction_id_);
            queue_.clear();
            Disconnect();
        }
    }
}

void ModbusTcpClient::GetValue(ModbusVariable variable) {
    uint16_t high_value{0};
    uint16_t low_value{0};
    uint8_t quality{0};
    uint64_t timestamp_receive{0};

    {
        std::lock_guard<std::mutex> lock(mutex_memory_);
        const auto &it1 = memory_.holding_registers.find(variable.addr);
        if (it1 != memory_.holding_registers.end()) {
            high_value = it1->second.value;
            quality = it1->second.quality;
            timestamp_receive = it1->second.timestamp_receive;
            if (variable.type == "DINT" || variable.type == "UDINT" || variable.type == "DWORD" || variable.type == "REAL") {
                const auto &it2 = memory_.holding_registers.find(variable.addr + 1);
                if (it2 != memory_.holding_registers.end()) {
                    low_value = it2->second.value;
                } else {
                    quality = 1;
                }
            }
        } else {
            quality = 1;
        }
    }

    if (quality == 0) {
        std::lock_guard<std::mutex> lock(mutex_variables_);
        if (variable.type == "INT") {
            variables_[variable.name].SetValue(static_cast<int>(high_value), timestamp_receive);
        }
        if (variable.type == "DINT") {
            variables_[variable.name].SetValue(static_cast<int>(high_value << 16 | low_value), timestamp_receive);
        }
        if (variable.type == "UINT" || variable.type == "WORD") {
            variables_[variable.name].SetValue(static_cast<unsigned int>(high_value), timestamp_receive);
        }
        if (variable.type == "UDINT" || variable.type == "DWORD") {
            variables_[variable.name].SetValue(static_cast<unsigned int>(high_value << 16 | low_value), timestamp_receive);
        }
        if (variable.type == "REAL") {
            variables_[variable.name].SetValue(static_cast<float>(high_value << 16 | low_value), timestamp_receive);
        }
    }
}
