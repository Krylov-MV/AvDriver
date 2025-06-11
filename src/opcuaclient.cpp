#include "opcuaclient.h"

bool OpcUaClient::Connect() {
    std::string url = "opc.tcp://" + ip_ + ":" + std::to_string(port_);

    char* opc_url = strdup(url.c_str());
    UA_StatusCode status_code = UA_Client_connect(client_, opc_url);
    free(opc_url);

    is_connected_ = status_code == UA_STATUSCODE_GOOD;
    return is_connected_;
}

bool OpcUaClient::CheckConnection() {
    return Connect();
}

void OpcUaClient::Disconnect() {
    UA_Client_disconnect(client_);
    is_connected_ = false;
}

void OpcUaClient::Stop() {
    should_run_ = false;
}

void OpcUaClient::Read(const ReadConfig& config, Value& result) {
    UA_ReadValueId item;

    UA_ReadRequest request;
    UA_ReadRequest_init(&request);

    UA_ReadValueId_init(&item);
    item.nodeId = UA_NODEID_STRING_ALLOC(1, config.node_id.c_str());
    item.attributeId = UA_ATTRIBUTEID_VALUE;

    request.nodesToRead = &item;
    request.nodesToReadSize = 1;

    UA_ReadResponse response;

    response = UA_Client_Service_read(client_, request);

    if (response.responseHeader.serviceResult == UA_STATUSCODE_GOOD) {
        if (response.results[0].hasValue && (response.results[0].status >= UA_STATUSCODE_GOOD && response.results[0].status <= UA_STATUSCODE_UNCERTAIN)) {
            Value value;
            value.source_timestamp = response.results[0].sourceTimestamp;
            value.quality = response.results[0].status;
            value.type = config.type;
            std::string node_id = config.node_id;
            if (config.type == "INT") {
                value.i = *(int*)response.results[0].value.data;
            }
            if (config.type == "DINT") {
                value.i = *(int*)response.results[0].value.data;
            }
            if (config.type == "UINT" || config.type == "WORD") {
                value.u = *(unsigned int*)response.results[0].value.data;
            }
            if (config.type == "UDINT" || config.type == "DWORD") {
                value.u = *(unsigned int*)response.results[0].value.data;
            }
            if (config.type == "REAL") {
                value.f = *(float*)response.results[0].value.data;
            }
            if (config.type == "STRING") {
                value.s = *(std::string*)response.results[0].value.data;
            }

            result = { value };
        }
    } else {
        Disconnect();
    }

    UA_ReadValueId_clear(&item);
    UA_ReadResponse_clear(&response);
}

void OpcUaClient::Read(const std::vector<ReadConfig>& configs, std::map<std::string, Value>& results) {
    int data_count = configs.size();
    UA_ReadValueId items[data_count];

    UA_ReadRequest request;
    UA_ReadRequest_init(&request);

    for (int i = 0; i < data_count; i++) {
        UA_ReadValueId_init(&items[i]);
        items[i].nodeId = UA_NODEID_STRING_ALLOC(1, configs[i].node_id.c_str());
        items[i].attributeId = UA_ATTRIBUTEID_VALUE;
    }

    request.nodesToRead = items;
    request.nodesToReadSize = data_count;

    UA_ReadResponse response;

    response = UA_Client_Service_read(client_, request);

    if (response.responseHeader.serviceResult == UA_STATUSCODE_GOOD) {
        for (int i = 0; i < data_count; i++) {
            if (response.results[i].hasValue && (response.results[i].status >= UA_STATUSCODE_GOOD && response.results[i].status <= UA_STATUSCODE_UNCERTAIN)) {
                Value value;
                value.source_timestamp = response.results[i].sourceTimestamp;
                value.quality = response.results[i].status;
                value.type = configs[i].type;
                std::string node_id = configs[i].node_id;
                if (configs[i].type == "INT") {
                    value.i = *(int*)response.results[i].value.data;
                }
                if (configs[i].type == "DINT") {
                    value.i = *(int*)response.results[i].value.data;
                }
                if (configs[i].type == "UINT" || configs[i].type == "WORD") {
                    value.u = *(unsigned int*)response.results[i].value.data;
                }
                if (configs[i].type == "UDINT" || configs[i].type == "DWORD") {
                    value.u = *(unsigned int*)response.results[i].value.data;
                }
                if (configs[i].type == "REAL") {
                    value.f = *(float*)response.results[i].value.data;
                }
                if (configs[i].type == "STRING") {
                    value.s = *(std::string*)response.results[i].value.data;
                }
                results[node_id] = { value };
            }
        }
    } else {
        Disconnect();
    }

    for (int i = 0; i < data_count; i++) { UA_ReadValueId_clear(&items[i]); }
    UA_ReadResponse_clear(&response);
}

void OpcUaClient::Write(WriteConfig& config) {
    UA_WriteRequest request;
    UA_WriteRequest_init(&request);
    UA_WriteValue item;

    if (auto iterator = types_.find(config.type) != types_.end() && config.allowed) {
        config.allowed = false;

        UA_WriteValue_init(&item);

        item.nodeId = UA_NODEID_STRING_ALLOC(1, config.node_id.c_str());
        item.attributeId = UA_ATTRIBUTEID_VALUE;

        if (config.type == "INT") {
            item.value.value.type = &UA_TYPES[UA_TYPES_INT16];
            item.value.value.storageType = UA_VARIANT_DATA_NODELETE;
            item.value.value.data = &config.value.i;
        }
        if (config.type == "DINT") {
            item.value.value.type = &UA_TYPES[UA_TYPES_INT32];
            item.value.value.storageType = UA_VARIANT_DATA_NODELETE;
            item.value.value.data = &config.value.i;
        }
         if (config.type == "UINT" || config.type == "WORD") {
            item.value.value.type = &UA_TYPES[UA_TYPES_UINT16];
            item.value.value.storageType = UA_VARIANT_DATA_NODELETE;
            item.value.value.data = &config.value.u;
        }
        if (config.type == "UDINT" || config.type == "DWORD") {
            item.value.value.type = &UA_TYPES[UA_TYPES_UINT32];
            item.value.value.storageType = UA_VARIANT_DATA_NODELETE;
            item.value.value.data = &config.value.u;
        }
        if (config.type == "REAL") {
            item.value.value.type = &UA_TYPES[UA_TYPES_FLOAT];
            item.value.value.storageType = UA_VARIANT_DATA_NODELETE;
            item.value.value.data = &config.value.f;
        }
        if (config.type == "STRING") {
            item.value.value.type = &UA_TYPES[UA_TYPES_STRING];
            item.value.value.storageType = UA_VARIANT_DATA_NODELETE;
            item.value.value.data = &config.value.s;
        }
        item.value.hasValue = true;
    }

    request.nodesToWrite = &item;
    request.nodesToWriteSize = 1;

    UA_WriteResponse response = UA_Client_Service_write(client_, request);

    if (response.responseHeader.serviceResult != UA_STATUSCODE_GOOD) {
        Disconnect();
    }

    UA_WriteResponse_clear(&response);

    UA_WriteValue_clear(&item);
}

void OpcUaClient::Write(std::vector<WriteConfig>& configs) {
    int data_count = configs.size();

    UA_WriteRequest request;
    UA_WriteRequest_init(&request);
    UA_WriteValue items[data_count];

    int j = 0;
    for (int i = 0; i < data_count; i++) {
        if (auto iterator = types_.find(configs[i].type) != types_.end() && configs[i].allowed) {
            configs[i].allowed = false;

            UA_WriteValue_init(&items[j]);

            items[j].nodeId = UA_NODEID_STRING_ALLOC(1, configs[i].node_id.c_str());
            items[j].attributeId = UA_ATTRIBUTEID_VALUE;

            if (configs[i].type == "INT") {
                items[j].value.value.type = &UA_TYPES[UA_TYPES_INT16];
                items[j].value.value.storageType = UA_VARIANT_DATA_NODELETE;
                items[j].value.value.data = &configs[i].value.i;
            }
            if (configs[i].type == "DINT") {
                items[j].value.value.type = &UA_TYPES[UA_TYPES_INT32];
                items[j].value.value.storageType = UA_VARIANT_DATA_NODELETE;
                items[j].value.value.data = &configs[i].value.i;
            }
             if (configs[i].type == "UINT" || configs[i].type == "WORD") {
                items[j].value.value.type = &UA_TYPES[UA_TYPES_UINT16];
                items[j].value.value.storageType = UA_VARIANT_DATA_NODELETE;
                items[j].value.value.data = &configs[i].value.u;
            }
            if (configs[i].type == "UDINT" || configs[i].type == "DWORD") {
                items[j].value.value.type = &UA_TYPES[UA_TYPES_UINT32];
                items[j].value.value.storageType = UA_VARIANT_DATA_NODELETE;
                items[j].value.value.data = &configs[i].value.u;
            }
            if (configs[i].type == "REAL") {
                items[j].value.value.type = &UA_TYPES[UA_TYPES_FLOAT];
                items[j].value.value.storageType = UA_VARIANT_DATA_NODELETE;
                items[j].value.value.data = &configs[i].value.f;
            }
            if (configs[i].type == "STRING") {
                items[j].value.value.type = &UA_TYPES[UA_TYPES_STRING];
                items[j].value.value.storageType = UA_VARIANT_DATA_NODELETE;
                items[j].value.value.data = &configs[i].value.s;
            }
            items[j].value.hasValue = true;

            j++;
        }
    }

    request.nodesToWrite = items;
    request.nodesToWriteSize = data_count;

    UA_WriteResponse response = UA_Client_Service_write(client_, request);

    if (response.responseHeader.serviceResult != UA_STATUSCODE_GOOD) {
        Disconnect();
    }

    UA_WriteResponse_clear(&response);

    for (int i = 0; i < j; i++) { UA_WriteValue_clear(&items[i]); }
}
