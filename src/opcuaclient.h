#ifndef OPCUACLIENT_H
#define OPCUACLIENT_H

#include <open62541/client.h>
#include <open62541/plugin/log_stdout.h>
#include <set>
#include <vector>
#include <variant>
#include <iostream>

#pragma once

class OpcUaClient {
public:
    OpcUaClient()
        : is_connected_(false), should_run_(true) {
        client_ = UA_Client_new();

        UA_ClientConfig* config = UA_Client_getConfig(client_);
        UA_Client_run_iterate(client_, 100);
        *config->logging = UA_Log_Stdout_withLevel(UA_LOGLEVEL_FATAL);
    }

    ~OpcUaClient() {
        should_run_ = false;
        Disconnect();
        UA_Client_delete(client_);
    }

    struct DeviceConfig {
        std::string url;
        uint timeout_reconnect;
        uint timeout_read_write;
    };

    struct WriteConfig {
        std::string node_id;
        std::string type;
        std::variant<int16_t, uint16_t, int32_t, uint32_t, float> value;
    };

    struct ReadConfig {
        std::string node_id;
        std::string type;
    };

    struct WriteReadConfigs {
        std::vector<WriteConfig> write;
        std::vector<ReadConfig> read;
    };

    struct ReadResult {
        std::variant<int16_t, uint16_t, int32_t, uint32_t, float> value;
        long source_timestamp;
        uint32_t quality;
        std::string node_id;
    };

    static int TypeLength(const std::string& type);

    void Read(const std::vector<ReadConfig>& configs, std::vector<ReadResult>& results);

    void Write(std::vector<WriteConfig>& configs);

    bool Connect();

    void Disconnect();

    bool CheckConnection();

private:
    std::string url_ = "opc.tcp://127.0.0.1:62544";
    UA_Client* client_;
    bool is_connected_;
    bool should_run_;
    static const std::set<std::string> types;

    void Stop();
};

#endif // OPCUACLIENT_H
