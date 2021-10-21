#include "IotaMessageToTransactionTask.h"

#include "../model/transactions/GradidoTransaction.h"
#include "../SingletonManager/LoggerManager.h"
#include "../SingletonManager/GroupManager.h"
#include "../lib/DataTypeConverter.h"

#include <stdexcept>

IotaMessageToTransactionTask::IotaMessageToTransactionTask(uint32_t milestoneIndex, uint64_t timestamp, const iota::MessageId& messageId)
: mMilestoneIndex(milestoneIndex), mTimestamp(timestamp), mMessageId(messageId)
{

}

IotaMessageToTransactionTask::~IotaMessageToTransactionTask()
{

}

int IotaMessageToTransactionTask::run()
{
    static const char* function_name = "IotaMessageToTransactionTask::run";
    Poco::Logger& errorLog = LoggerManager::getInstance()->mErrorLogging;
    if (!mMessageId.groupAlias) {
        errorLog.error("[%s] no group alias, abort", function_name);
        return 0;
    }
    // get the whole transaction from iota
    auto binString = iota::getIndexiationMessage(mMessageId);
    // parse proto Object from it and
    auto gm = GroupManager::getInstance();
    auto group = gm->findGroup(mMessageId.groupAlias);
    if (group.isNull()) {
        errorLog.error("[%s] group with alias: %s not found on server, abort", function_name, mMessageId.groupAlias);
        throw new std::runtime_error("group alias not found, but node server should only listen on existing groups");
    }
    Poco::AutoPtr<model::GradidoTransaction> transaction(new model::GradidoTransaction(binString, group));
    if(transaction->errorCount() > 0) {
        auto errors = transaction->getErrorsArray();
        errorLog.error("Error by parsing transaction");
        errorLog.error("Transaction as base64: %s", DataTypeConverter::binToBase64(binString));
        for(auto it = errors.begin(); it != errors.end();  it++) {
            errorLog.error(*it);
        }
        
    } else {
        // hand over to OrderingManager
        std::clog << "transaction: " << std::endl << transaction->getJson() << std::endl;
    }

    return 0;
}
