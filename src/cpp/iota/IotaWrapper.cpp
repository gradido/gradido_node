#include "IotaWrapper.h"

#ifdef __linux__
#include "client/api/v1/get_message.h"
#include "client/api/v1/find_message.h"
#include "client/api/v1/get_node_info.h"
#include <inttypes.h>
#endif

#include "../SingletonManager/LoggerManager.h"
#include "../lib/BinTextConverter.h"
#include "../ServerGlobals.h"
#include <sstream>
#include <assert.h>


namespace iota
{
#ifdef __linux__
    std::string toErrorString(res_err_t* iotaResponseError)
    {
        std::stringstream ss;
        ss << "Iota Error: " << iotaResponseError->msg << ", code: " << iotaResponseError->code << std::endl;
        return ss.str();
    }
#endif

    std::vector<MessageId> getMessageIdsForIndexiation(const std::string& indexiation)
    {
        Poco::Logger& errorLog = LoggerManager::getInstance()->mErrorLogging;
        static const char* functionName = "iota::getMessageIdsForIndexiation";
        std::vector<MessageId> result;
#ifdef __linux__
        res_find_msg_t* iotaResult = res_find_msg_new();
        if(!iotaResult) {
            errorLog.error("[%s] couldn't get memory for message response", functionName);
            return result;
        }

        if(!find_message_by_index(&ServerGlobals::g_IotaClientConfig, indexiation.data(), iotaResult)) {
            // success
            if(iotaResult->is_error) {
                errorLog.error("[%s] find some, but with error: %s", std::string(functionName), toErrorString(iotaResult->u.error));
            }
            auto count = res_find_msg_get_id_len(iotaResult);
            if(count > 0) {
                printf("find %d messages\n", count);
            }
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
#endif
        return result;
    }

    Milestone getMilestone(MessageId milestoneMessageId)
    {
        Poco::Logger& errorLog = LoggerManager::getInstance()->mErrorLogging;
        static const char* functionName = "iota::getMilestone";
        Milestone result;
#ifdef __linux__
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
#endif
        return result;
    }

    std::string getIndexiationMessage(MessageId indexiationMessageId)
    {
        Poco::Logger& errorLog = LoggerManager::getInstance()->mErrorLogging;
        static const char* functionName = "iota::getIndexiationMessage";
        std::string result = "";
#ifdef __linux__
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
                    // iota encode the payload as hex, protobuf need binary in a string
                    result = convertHexToBin((const char*)payload->data->data);
                }
            }
        } else {
            errorLog.error("[%s] get_message_by_id API failed", functionName);
        }

        res_message_free(msg);
#endif
        return result;
    }

    NodeInfo getNodeInfo()
    {
        NodeInfo result;
#ifdef __linux__
        res_node_info_t *info = res_node_info_new();
        if (info)
        {
            int ret = get_node_info(&ServerGlobals::g_IotaClientConfig, info);
            if (ret == 0)
            {
              if (!info->is_error)
              {
                result.confirmedMilestoneIndex = info->u.output_node_info->confirmed_milestone_index;
                result.lastMilestoneIndex = info->u.output_node_info->latest_milestone_index;
                result.lastMilestoneTimestamp = info->u.output_node_info->latest_milestone_timestamp;

                /*
                printf("Name: %s\n", info->u.output_node_info->name);
                printf("Version: %s\n", info->u.output_node_info->version);
                printf("isHealthy: %s\n", info->u.output_node_info->is_healthy ? "true" : "false");
                printf("Network ID: %s\n", info->u.output_node_info->network_id);
                printf("bech32HRP: %s\n", info->u.output_node_info->bech32hrp);
                printf("minPoWScore: %" PRIu64 "\n", info->u.output_node_info->min_pow_score);
                printf("Latest Milestone Index: %" PRIu64 "\n", info->u.output_node_info->latest_milestone_index);
                printf("Latest Milestone Timestamp: %" PRIu64 "\n", info->u.output_node_info->latest_milestone_timestamp);
                printf("Confirmed Milestone Index: %" PRIu64 "\n", info->u.output_node_info->confirmed_milestone_index);
                printf("Pruning Index: %" PRIu64 "\n", info->u.output_node_info->pruning_milestone_index);
                printf("MSP: %0.2f\n", info->u.output_node_info->msg_pre_sec);
                printf("Referenced MPS: %0.2f\n", info->u.output_node_info->referenced_msg_pre_sec);
                printf("Reference Rate: %0.2f%%\n", info->u.output_node_info->referenced_rate);
                */
              } else {
                printf("Node response: %s\n", info->u.error->msg);
              }
            } else {
              printf("get node info API failed\n");
            }
            res_node_info_free(info);
          } else {
            printf("new respose object failed\n");
          }
#endif
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
