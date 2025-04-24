#include "opcuaclient.h"

void OpcUaClient::ReadDatas(const std::vector<IndustrialProtocolUtils::DataConfig>& data_configs, std::vector<IndustrialProtocolUtils::DataResult>& data_results) {
    int data_count = data_configs.size();
    UA_ReadValueId items[data_count];

    UA_ReadRequest request;
    UA_ReadRequest_init(&request);

    for (int i = 0; i < data_count; i++) {
        UA_ReadValueId_init(&items[i]);
        items[i].nodeId = UA_NODEID_STRING(1, strdup(data_configs[i].name.c_str()));
        items[i].attributeId = UA_ATTRIBUTEID_VALUE;
    }

    request.nodesToRead = items;
    request.nodesToReadSize = data_count;

    UA_ReadResponse response;

    response = UA_Client_Service_read(client_, request);

    if (response.responseHeader.serviceResult == UA_STATUSCODE_GOOD) {
        for (int i = 0; i < data_count; i++) {
            if (response.results[i].hasValue) {
                data_results[i].address = data_configs[i].address;
                data_results[i].name = data_configs[i].name;
                data_results[i].time_current = response.results[i].sourceTimestamp;
                data_results[i].type = data_configs[i].type;
                switch (data_configs[i].type)
                {
                case (IndustrialProtocolUtils::DataType::INT):
                    data_results[i].value.i = *(int*)response.results[i].value.data;
                case (IndustrialProtocolUtils::DataType::DINT):
                    data_results[i].value.i = *(int*)response.results[i].value.data;
                case (IndustrialProtocolUtils::DataType::UINT):
                case (IndustrialProtocolUtils::DataType::WORD):
                    data_results[i].value.u = *(uint*)response.results[i].value.data;
                case (IndustrialProtocolUtils::DataType::UDINT):
                case (IndustrialProtocolUtils::DataType::DWORD):
                    data_results[i].value.u = *(uint*)response.results[i].value.data;
                case (IndustrialProtocolUtils::DataType::REAL):
                    data_results[i].value.f = *(UA_Float*)response.results[i].value.data;
                }
            }
        }
    }

    for (int i = 0; i < data_count; i++) { UA_ReadValueId_clear(&items[i]); }
    UA_ReadResponse_clear(&response);
}

void OpcUaClient::WriteDatas(std::vector<IndustrialProtocolUtils::DataResult>& datas) {
    int data_count = datas.size();

    UA_WriteRequest request;
    UA_WriteRequest_init(&request);
    UA_WriteValue items[data_count];

    for (int i = 0; i < data_count; i++) {
        UA_WriteValue_init(&items[i]);

        items[i].nodeId = UA_NODEID_STRING(1, strdup(datas[i].name.c_str()));
        items[i].attributeId = UA_ATTRIBUTEID_VALUE;

        if (datas[i].type == IndustrialProtocolUtils::DataType::INT) {
            items[i].value.value.type = &UA_TYPES[UA_TYPES_INT16];
            items[i].value.value.storageType = UA_VARIANT_DATA_NODELETE;
            items[i].value.value.data = &datas[i].value.i;
        }
        if (datas[i].type == IndustrialProtocolUtils::DataType::DINT) {
            items[i].value.value.type = &UA_TYPES[UA_TYPES_INT32];
            items[i].value.value.storageType = UA_VARIANT_DATA_NODELETE;
            items[i].value.value.data = &datas[i].value.i;
        }
        if (datas[i].type == IndustrialProtocolUtils::DataType::UINT) {
            items[i].value.value.type = &UA_TYPES[UA_TYPES_UINT16];
            items[i].value.value.storageType = UA_VARIANT_DATA_NODELETE;
            items[i].value.value.data = &datas[i].value.u;
        }
        if (datas[i].type == IndustrialProtocolUtils::DataType::WORD) {
            items[i].value.value.type = &UA_TYPES[UA_TYPES_UINT16];
            items[i].value.value.storageType = UA_VARIANT_DATA_NODELETE;
            items[i].value.value.data = &datas[i].value.u;
        }
        if (datas[i].type == IndustrialProtocolUtils::DataType::UDINT) {
            items[i].value.value.type = &UA_TYPES[UA_TYPES_UINT32];
            items[i].value.value.storageType = UA_VARIANT_DATA_NODELETE;
            items[i].value.value.data = &datas[i].value.u;
        }
        if (datas[i].type == IndustrialProtocolUtils::DataType::DWORD) {
            items[i].value.value.type = &UA_TYPES[UA_TYPES_UINT32];
            items[i].value.value.storageType = UA_VARIANT_DATA_NODELETE;
            items[i].value.value.data = &datas[i].value.u;
        }
        if (datas[i].type == IndustrialProtocolUtils::DataType::REAL) {
            items[i].value.value.type = &UA_TYPES[UA_TYPES_FLOAT];
            items[i].value.value.storageType = UA_VARIANT_DATA_NODELETE;
            items[i].value.value.data = &datas[i].value.f;
        }
        items[i].value.hasValue = true;

    }

    request.nodesToWrite = items;
    request.nodesToWriteSize = data_count;

    UA_WriteResponse response = UA_Client_Service_write(client_, request);

    UA_WriteResponse_clear(&response);
    for (int i = 0; i < data_count; i++) { UA_WriteValue_clear(&items[i]); }
}

void OpcUaClient::WriteDatas(const std::vector<IndustrialProtocolUtils::DataConfig>& data_configs, std::vector<IndustrialProtocolUtils::DataResult>& datas) {
    int data_count = data_configs.size();

    UA_WriteValue items[data_count];

    for (int i = 0; i < data_count; i++) {
        UA_Variant value;

        UA_WriteValue_init(&items[i]);

        items[i].nodeId = UA_NODEID_STRING(1, strdup(data_configs[i].name.c_str()));
        items[i].attributeId = UA_ATTRIBUTEID_VALUE;

        if (data_configs[i].type == IndustrialProtocolUtils::DataType::INT) {
            UA_Variant_setScalar(&value, &datas[i].value.i, &UA_TYPES[UA_TYPES_INT16]);
        }
        if (data_configs[i].type == IndustrialProtocolUtils::DataType::DINT) {
            UA_Variant_setScalar(&value, &datas[i].value.i, &UA_TYPES[UA_TYPES_INT32]);
        }
        if (data_configs[i].type == IndustrialProtocolUtils::DataType::UINT) {
            UA_Variant_setScalar(&value, &datas[i].value.u, &UA_TYPES[UA_TYPES_UINT16]);
        }
        if (data_configs[i].type == IndustrialProtocolUtils::DataType::WORD) {
            UA_Variant_setScalar(&value, &datas[i].value.u, &UA_TYPES[UA_TYPES_UINT16]);
        }
        if (data_configs[i].type == IndustrialProtocolUtils::DataType::UDINT) {
            UA_Variant_setScalar(&value, &datas[i].value.u, &UA_TYPES[UA_TYPES_UINT32]);
        }
        if (data_configs[i].type == IndustrialProtocolUtils::DataType::DWORD) {
            UA_Variant_setScalar(&value, &datas[i].value.u, &UA_TYPES[UA_TYPES_UINT32]);
        }
        if (data_configs[i].type == IndustrialProtocolUtils::DataType::REAL) {
            UA_Variant_setScalar(&value, &datas[i].value.f, &UA_TYPES[UA_TYPES_FLOAT]);
        }
        if (datas[i].type == IndustrialProtocolUtils::DataType::STRING) {
            UA_Variant_setScalar(&value, &datas[i].value.s, &UA_TYPES[UA_TYPES_STRING]);
        }
        items[i].value.value = value;
        items[i].value.hasValue = true;
    }

    UA_WriteRequest request;
    UA_WriteRequest_init(&request);

    request.nodesToWrite = items;
    request.nodesToWriteSize = data_count;

    UA_WriteResponse response = UA_Client_Service_write(client_, request);
}

bool OpcUaClient::Connect() {
    char* opc_url = strdup(ip_.c_str());
    UA_StatusCode status_code = UA_Client_connect(client_, opc_url);

    is_connected_ = status_code == UA_STATUSCODE_GOOD;
    return is_connected_;
}

bool OpcUaClient::Connect(const std::string ip, int port) {
    ip_ = ip;
    port_ = port;

    char* opc_url = strdup(ip_.c_str());
    UA_StatusCode status_code = UA_Client_connect(client_, opc_url);

    is_connected_ = status_code = UA_STATUSCODE_GOOD;
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
