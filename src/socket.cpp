#include "socket.h"
#include <unistd.h>

Socket::Socket(const std::string ip, const int port, const int timeout) : ip_(ip), port_(port), timeout_(timeout) {}

bool Socket::Connect() {
    //Открываем сокет
    socket_ = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_ < 0) {
        Log("Ошибка открытия сокета \n");
        return false;
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
        return false;
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
        return false;
    }

    // Установка таймаута для записи
    if (setsockopt(socket_, SOL_SOCKET, SO_SNDTIMEO, (const char*)&timeout, sizeof(timeout)) < 0) {
        Log("Ошибка установки таймаута для записи \n");
        shutdown(socket_, SHUT_RDWR);
        close(socket_);
        return false;
    }

    return true;
}

void Socket::Disconnect() {
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
}
