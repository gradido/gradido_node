#include "IotaWrapper.h"

#ifdef __linux__
#include "client/api/v1/get_message.h"
#include "client/api/v1/find_message.h"
#endif

#include "../SingletonManager/LoggerManager.h"
#include "../lib/DataTypeConverter.h"
#include "../ServerGlobals.h"
#include <sstream>

namespace iota
{

    std::string toErrorString(res_err_t* iotaResponseError)
    {
        std::stringstream ss;
        ss << "Iota Error: " << iotaResponseError->msg << ", code: " << iotaResponseError->code << std::endl;
        return ss.str();
    }

    std::vector<MessageId> getMessageIdsForIndexiation(const std::string& indexiation)
    {
        Poco::Logger& errorLog = LoggerManager::getInstance()->mErrorLogging;
        static const char* functionName = "iota::getMessageIdsForIndexiation";

        std::vector<MessageId> result;

        res_find_msg_t* iotaResult = res_find_msg_new();
        if(!iotaResult) {
            errorLog.error("[%s] couldn't get memory for message response", functionName);
        }

        if(!find_message_by_index(&ServerGlobals::g_IotaClientConfig, indexiation.data(), iotaResult)) {
            // success
            if(iotaResult->is_error){
                errorLog.error("[%s] find some, but with error: %s", functionName, toErrorString(iotaResult->u.error));
            }
            auto count = res_find_msg_get_id_len(iotaResult);
            result.reserve(count);
            for(int i = 0; i < count; i++) {
                MessageId messageId;
                messageId.fromByteArray(res_find_msg_get_id(iotaResult, i));
                result.push_back(messageId);
            }
        } else if(iotaResult->is_error){
            errorLog.error("[%s] nothing found with error: %s", functionName, toErrorString(iotaResult->u.error));
        }
        res_find_msg_free(iotaResult);
        return result;
    }

    Milestone getMilestone(MessageId milestoneMessageId)
    {
        Poco::Logger& errorLog = LoggerManager::getInstance()->mErrorLogging;
        static const char* functionName = "iota::getMilestone";
        Milestone result;

        res_message_t *msg = res_message_new();
        if(!msg) {
            errorLog.error("[%s] couldn't get memory for message response", functionName);
        }

        if (!get_message_by_id(&ServerGlobals::g_IotaClientConfig, (const char*)milestoneMessageId.messageId, msg))
        {
            if (msg->is_error) {
                errorLog.error("[%s] API response: %s", functionName, msg->u.error->msg);
            } else {
                if(MSG_PAYLOAD_MILESTONE != msg->u.msg->type) {
                    errorLog.error("[%s] wrong message type: %d", functionName, msg->u.msg->type);
                } else {
                    auto payload = (payload_milestone_t*)msg->u.msg->payload;
                    result.timestamp = payload->timestamp;
                    result.id = payload->index;
                    auto count = api_message_parent_count(msg->u.msg);
                    result.messageIds.reserve(count);
                    for(int i = 0; i < count; i++) {
                        MessageId messageId;
                        messageId.fromByteArray(api_message_parent_id(msg->u.msg, i));
                        result.messageIds.push_back(messageId);
                    }
                }
            }
        } else {
            errorLog.error("[%s] get_message_by_id API failed", functionName);
        }

        res_message_free(msg);
        return result;
    }

    MemoryBin* getIndexiationMessage(MessageId indexiationMessageId)
    {
        Poco::Logger& errorLog = LoggerManager::getInstance()->mErrorLogging;
        static const char* functionName = "iota::getIndexiationMessage";
        MemoryBin* result = nullptr;

        res_message_t *msg = res_message_new();
        if(!msg) {
            errorLog.error("[%s] couldn't get memory for message response", functionName);
        }

        if (!get_message_by_id(&ServerGlobals::g_IotaClientConfig, (const char*)indexiationMessageId.messageId, msg))
        {
            if (msg->is_error) {
                errorLog.error("[%s] API response: %s", functionName, msg->u.error->msg);
            } else {
                if(MSG_PAYLOAD_INDEXATION != msg->u.msg->type) {
                    errorLog.error("[%s] wrong message type: %d", functionName, msg->u.msg->type);
                } else {
                    auto payload = (payload_index_t*)msg->u.msg->payload;
                    // make binary from hex and copy it over
                    result = DataTypeConverter::hexToBin((const char*)payload->data->data);
                }
            }
        } else {
            errorLog.error("[%s] get_message_by_id API failed", functionName);
        }

        res_message_free(msg);
        return result;
    }



    // ****************** Message Id ******************************
    void MessageId::fromByteArray(const char* byteArray)
    {
        assert(byteArray);
        memcpy(messageId, byteArray, 32);
    }

    MemoryBin* MessageId::toMemoryBin()
    {
        auto result = MemoryManager::getInstance()->getFreeMemory(32);
        memcpy(result->data(), messageId, 32);
        return result;
    }
}
