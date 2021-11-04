#include "IotaMessageToTransactionTask.h"

#include "../model/transactions/GradidoTransaction.h"
#include "../SingletonManager/LoggerManager.h"
#include "../SingletonManager/GroupManager.h"
#include "../SingletonManager/OrderingManager.h"
#include "../lib/DataTypeConverter.h"

IotaMessageToTransactionTask::IotaMessageToTransactionTask(
    uint32_t milestoneIndex, uint64_t timestamp, 
    Poco::SharedPtr<iota::Message> message)
: mMilestoneIndex(milestoneIndex), mTimestamp(timestamp), mMessage(message)
{
#ifdef _UNI_LIB_DEBUG
    setName(mMessage->getId().toHex().data());
#endif
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
    auto gm = GroupManager::getInstance();
    auto group = gm->findGroup(mMessage->getGradidoGroupAlias());
    
    Poco::AutoPtr<model::GradidoTransaction> transaction(new model::GradidoTransaction(binString, group));
    if(transaction->errorCount() > 0) {
        auto errors = transaction->getErrorsArray();
        errorLog.error("Error by parsing transaction");
        errorLog.error("Transaction as base64: %s", DataTypeConverter::binToBase64(binString));
        for(auto it = errors.begin(); it != errors.end();  it++) {
            errorLog.error(*it);
        }
        
    } else {		
		// if it is a cross group transaction we store both in Ordering Manager for easy access for validation
		if (transaction->getTransactionBody()->isTransfer()) {
			OrderingManager::getInstance()->pushPairedTransaction(transaction);
		}
        // check if transaction already exist
        // if this transaction doesn't belong to us, we can quit here 
        if (group.isNull() || group->isSignatureInCache(transaction)) {
            return 0;
        }       
        
        // hand over to OrderingManager
        std::clog << "transaction: " << std::endl << transaction->getJson() << std::endl;
        OrderingManager::getInstance()->pushTransaction(transaction, mMilestoneIndex);
    }

    return 0;
}
