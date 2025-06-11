#ifndef OPCUACLIENT_H
#define OPCUACLIENT_H

#include <open62541/client.h>
#include <open62541/plugin/log_stdout.h>
#include <vector>
#include <map>
#include <set>
#include <memory>
#include <string>

#pragma once

class OpcUaClient {
public:
    OpcUaClient(const std::string &ip, int port)
        : ip_(ip), port_(port), is_connected_(false), should_run_(true) {
        client_ = UA_Client_new();

        UA_ClientConfig* config = UA_Client_getConfig(client_);
        *config->logging = UA_Log_Stdout_withLevel(UA_LOGLEVEL_FATAL);

        UA_Client_run_iterate(client_, 100);

        Connect();
    }

    ~OpcUaClient() {
        should_run_ = false;
        Disconnect();
        UA_Client_delete(client_);
    }

    struct Value {
        union {
            int i;
            unsigned int u;
            float f;
        };
        std::string s;
        std::string type;
        long source_timestamp;
        uint32_t quality;
    };

    struct WriteConfig {
        bool allowed;
        std::string node_id;
        std::string type;
        Value value;
    };

    struct ReadConfig {
        std::string node_id;
        std::string type;
    };

    bool Connect();
    bool CheckConnection();
    void Disconnect();
    void Write(WriteConfig& config);
    void Write(std::vector<WriteConfig>& configs);
    void Read(const ReadConfig& config, Value& result);
    void Read(const std::vector<ReadConfig>& configs, std::map<std::string, Value>& results);

private:
    std::string ip_{"127.0.0.1"};
    int port_{62544};
    UA_Client* client_;
    bool is_connected_{false};
    bool should_run_{false};
    const std::set<std::string> types_ = { "INT", "UINT", "WORD", "DINT", "UDINT", "DWORD", "REAL" };

private:
    void Stop();
};

#endif // OPCUACLIENT_H
