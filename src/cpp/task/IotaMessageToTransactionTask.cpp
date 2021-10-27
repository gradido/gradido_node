#include "IotaMessageToTransactionTask.h"

#include "../model/transactions/GradidoTransaction.h"
#include "../SingletonManager/LoggerManager.h"
#include "../SingletonManager/GroupManager.h"
#include "../SingletonManager/OrderingManager.h"
#include "../lib/DataTypeConverter.h"

#include <stdexcept>

IotaMessageToTransactionTask::IotaMessageToTransactionTask(
    uint32_t milestoneIndex, uint64_t timestamp, 
    Poco::SharedPtr<iota::Message> message, Poco::SharedPtr<controller::Group> group)
: mMilestoneIndex(milestoneIndex), mTimestamp(timestamp), mMessage(message), mGroup(group)
{
    OrderingManager::getInstance()->pushMilestoneTaskObserver(milestoneIndex, mTimestamp);
}

IotaMessageToTransactionTask::~IotaMessageToTransactionTask()
{
    OrderingManager::getInstance()->popMilestoneTaskObserver(mMilestoneIndex);
}

int IotaMessageToTransactionTask::run()
{
    static const char* function_name = "IotaMessageToTransactionTask::run";
    Poco::Logger& errorLog = LoggerManager::getInstance()->mErrorLogging;
    
    auto binString = mMessage->getDataString();

    if (mGroup.isNull()) {
        errorLog.error("[%s] group not found on server, abort", function_name);
        throw new std::runtime_error("group alias not found, but node server should only listen on existing groups");
    }
    Poco::AutoPtr<model::GradidoTransaction> transaction(new model::GradidoTransaction(binString, mGroup));
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
        OrderingManager::getInstance()->pushTransaction(transaction, mMilestoneIndex);
    }

    return 0;
}
