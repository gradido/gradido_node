#include "IotaWrapper.h"

#ifdef __linux__
#include "client/api/v1/get_message.h"
#endif 

#include "../SingletonManager/LoggerManager.h"
#include <sstream>

namespace iota
{
    std::vector<MemoryBin*> getMessageIdsForIndexiation(const std::string& indexiation)
    {
        std::vector<MemoryBin*> result;

        res_find_msg_t* iotaResult = res_find_msg_new();
        
        if(!find_message_by_index(&ServerGlobals::g_IotaClientConfig, mIndex.data(), &iotaResult)) {
            // success
            auto count = res_find_msg_get_id_len(&iotaResult);
            result.resize(count);
            for(int i = 0; i < count; i++) {
                auto messageIdBin = mm->getFreeMemory(32);
                IotaMessageId messageId;
                messageId.fromByteArray(res_find_msg_get_id(&iotaResult, i));
                result.push_back(messageId);
            }            
        } else {
            LoggerManager::getInstance()->mErrorLogging.error(
                "[%s] %s", 
                getMessageIdsForIndexiation,
                toErrorString(iotaResult.u.error)
            );
        }
        res_find_msg_free(iotaResult);
        return result;
    }

    std::string toErrorString(res_err_t* iotaResponseError)
    {
        std::stringstream ss;
        ss << "Iota Error: " << iotaResponseError->msg << ", code: " << iotaResponseError->code << std::endl;
        return ss.str();
    }

    // ****************** Message Id ******************************
    void IotaMessageId::fromByteArray(const char* byteArray)
    {
        assert(byteArray);
        memcpy(messageId, byteArray, 32);
    }

    MemoryBin* IotaMessageId::toMemoryBin()
    {
        auto result = MemoryManager::getInstance()->getFreeMemory(32);
        memcpy(result.data(), messageId, 32);
        return result;
    }
}