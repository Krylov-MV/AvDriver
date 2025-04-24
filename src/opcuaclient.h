#ifndef OPCUACLIENT_H
#define OPCUACLIENT_H

#include "industrialprotocolutils.h"
#include <open62541.h>
#include <vector>

#pragma once

class OpcUaClient {
public:
    OpcUaClient(const std::string &ip, int port)
        : ip_(ip), port_(port), is_connected_(false), should_run_(true) {
        client_ = UA_Client_new();

        UA_ClientConfig* config = UA_Client_getConfig(client_);
        UA_ClientConfig_setDefault(config);

        config->logger = UA_Log_Stdout_withLevel(UA_LOGLEVEL_FATAL);
        config->securityMode = UA_MESSAGESECURITYMODE_NONE;
        config->securityPolicyUri = UA_STRING_ALLOC("http://opcfoundation.org/UA/SecurityPolicy#None");

        Connect();
    }

    OpcUaClient()
        : is_connected_(false), should_run_(true) {
        client_ = UA_Client_new();

        UA_ClientConfig* config = UA_Client_getConfig(client_);
        UA_ClientConfig_setDefault(config);

        config->logger = UA_Log_Stdout_withLevel(UA_LOGLEVEL_FATAL);
        config->securityMode = UA_MESSAGESECURITYMODE_NONE;
        config->securityPolicyUri = UA_STRING_ALLOC("http://opcfoundation.org/UA/SecurityPolicy#None");
    }

    ~OpcUaClient() {
        should_run_ = false;
        Disconnect();
        UA_Client_delete(client_);
    }

    void ReadDatas(const std::vector<IndustrialProtocolUtils::DataConfig>& data_configs, std::vector<IndustrialProtocolUtils::DataResult>& data_results);

    void WriteDatas(std::vector<IndustrialProtocolUtils::DataResult>& datas);

    void WriteDatas(const std::vector<IndustrialProtocolUtils::DataConfig>& data_configs, std::vector<IndustrialProtocolUtils::DataResult>& datas);

    bool Connect();

    bool Connect(const std::string ip, int port);

    void Disconnect();

    bool CheckConnection();

private:
    std::string ip_ = "opc.tcp://127.0.0.1:62544";
    int port_ = 62544;
    UA_Client* client_;
    bool is_connected_;
    bool should_run_;

    void Stop();
};

#endif // OPCUACLIENT_H
