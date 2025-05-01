#include "opcuaclient.h"

const std::set<std::string> OpcUaClient::types = { "INT", "UINT", "WORD", "DINT", "UDINT", "DWORD", "REAL" };

int OpcUaClient::TypeLength(const std::string& type) {
    if (!types.contains(type)) {
        return 0;
    }  else if (type == "INT" || type == "UINT" || type == "WORD") {
        return 1;
    }  else if (type == "DINT" || type == "UDINT" || type == "DWORD" || type == "REAL") {
        return 2;
    }
}

void OpcUaClient::Read(const std::vector<ReadConfig>& configs, std::map<std::string, OpcUaValue>& results) {
    int data_count = configs.size();
    UA_ReadValueId items[data_count];

    UA_ReadRequest request;
    UA_ReadRequest_init(&request);

    for (int i = 0; i < data_count; i++) {
        UA_ReadValueId_init(&items[i]);
        //items[i].nodeId = UA_NODEID_STRING(1, strdup(configs[i].node_id.c_str()));
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
                long source_timestamp = response.results[i].sourceTimestamp;
                uint32_t quality = response.results[i].status;
                std::string node_id = configs[i].node_id;
                std::variant<int16_t, uint16_t, int32_t, uint32_t, float> value;
                if (configs[i].type == "INT") {
                    value = *(int16_t*)response.results[i].value.data;
                }
                if (configs[i].type == "DINT") {
                    value = *(int32_t*)response.results[i].value.data;
                }
                if (configs[i].type == "UINT" || configs[i].type == "WORD") {
                    value = *(uint16_t*)response.results[i].value.data;
                }
                if (configs[i].type == "UDINT" || configs[i].type == "DWORD") {
                    value = *(uint32_t*)response.results[i].value.data;
                }
                if (configs[i].type == "REAL") {
                    value = *(float*)response.results[i].value.data;
                }

                results[node_id] = {value, source_timestamp, quality};
            }
        }
    } else {
        Disconnect();
    }

    for (int i = 0; i < data_count; i++) { UA_ReadValueId_clear(&items[i]); }
    UA_ReadResponse_clear(&response);
}

void OpcUaClient::Write(std::vector<WriteConfig>& configs) {
    int data_count = configs.size();

    UA_WriteRequest request;
    UA_WriteRequest_init(&request);
    UA_WriteValue items[data_count];

    int j = 0;
    for (int i = 0; i < data_count; i++) {
        if (types.contains(configs[i].type) && configs[i].allowed) {
            configs[i].allowed = false;

            UA_WriteValue_init(&items[j]);

            //items[j].nodeId = UA_NODEID_STRING(1, strdup(configs[i].node_id.c_str()));
            items[j].nodeId = UA_NODEID_STRING_ALLOC(1, configs[i].node_id.c_str());
            items[j].attributeId = UA_ATTRIBUTEID_VALUE;

            if (configs[i].type == "INT") {
                items[j].value.value.type = &UA_TYPES[UA_TYPES_INT16];
                items[j].value.value.storageType = UA_VARIANT_DATA_NODELETE;
                int16_t& value = std::get<int16_t>(configs[i].value);
                items[j].value.value.data = &value;
            }
            if (configs[i].type == "DINT") {
                items[j].value.value.type = &UA_TYPES[UA_TYPES_INT32];
                items[j].value.value.storageType = UA_VARIANT_DATA_NODELETE;
                int32_t& value = std::get<int32_t>(configs[i].value);
                items[j].value.value.data = &value;
            }
             if (configs[i].type == "UINT" || configs[i].type == "WORD") {
                items[j].value.value.type = &UA_TYPES[UA_TYPES_UINT16];
                items[j].value.value.storageType = UA_VARIANT_DATA_NODELETE;
                uint16_t& value = std::get<uint16_t>(configs[i].value);
                items[j].value.value.data = &value;
            }
            if (configs[i].type == "UDINT" || configs[i].type == "DWORD") {
                items[j].value.value.type = &UA_TYPES[UA_TYPES_UINT32];
                items[j].value.value.storageType = UA_VARIANT_DATA_NODELETE;
                uint32_t& value = std::get<uint32_t>(configs[i].value);
                items[j].value.value.data = &value;
            }
            if (configs[i].type == "REAL") {
                items[j].value.value.type = &UA_TYPES[UA_TYPES_FLOAT];
                items[j].value.value.storageType = UA_VARIANT_DATA_NODELETE;
                float& value = std::get<float>(configs[i].value);
                items[j].value.value.data = &value;
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

void OpcUaClient::SetUrl(const std::string& url) {
    url_ = url;
}

bool OpcUaClient::Connect() {
    UA_StatusCode status_code = UA_Client_connect(client_, url_.c_str());

    is_connected_ = status_code == UA_STATUSCODE_GOOD;
    return is_connected_;
}

void OpcUaClient::Disconnect() {
    UA_Client_disconnect(client_);
    is_connected_ = false;
}

bool OpcUaClient::CheckConnection() {
    return Connect();
}

void OpcUaClient::Stop() {
    should_run_ = false;
}
