#ifndef SOCKET_H
#define SOCKET_H

#pragma once

#include "industrialprotocolutils.h"
#include <arpa/inet.h>
#include <string>

class Socket
{
public:
    Socket(const std::string ip, const int port, const int timeout);
    bool Connect();
    void Disconnect();

private:
    std::string ip_;
    int port_;
    int timeout_;
    int socket_;
    sockaddr_in sockaddr_in_;
};

#endif // SOCKET_H
