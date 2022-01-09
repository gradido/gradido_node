#include "IotaMessageToTransactionTask.h"

#include "../model/transactions/GradidoTransaction.h"

#include "../SingletonManager/LoggerManager.h"
#include "../SingletonManager/GroupManager.h"
#include "../SingletonManager/OrderingManager.h"

#include "../lib/DataTypeConverter.h"
#include "../lib/BinTextConverter.h"

#include "../iota/HTTPApi.h"

IotaMessageToTransactionTask::IotaMessageToTransactionTask(
    uint32_t milestoneIndex, uint64_t timestamp, 
    iota::MessageId messageId)
: mMilestoneIndex(milestoneIndex), mTimestamp(timestamp), mMessageId(messageId)
{
#ifdef _UNI_LIB_DEBUG
    setName(messageId.toHex().data());
#endif
 
}

IotaMessageToTransactionTask::~IotaMessageToTransactionTask()
{
    OrderingManager::getInstance()->popMilestoneTaskObserver(mMilestoneIndex);
}

int IotaMessageToTransactionTask::run()
{
    static const char* function_name = "IotaMessageToTransactionTask::run";
    Poco::Logger& errorLog = LoggerManager::getInstance()->mErrorLogging;
    
    auto dataIndex = iota::getIndexiationMessageDataIndex(mMessageId);

    auto binString = convertHexToBin(dataIndex.first);
    auto gm = GroupManager::getInstance();
    auto group = gm->findGroup(getGradidoGroupAlias(dataIndex.second));
    
    Poco::AutoPtr<model::GradidoTransaction> transaction(new model::GradidoTransaction(binString, group));
    if(transaction->errorCount() > 0) {
        auto errors = transaction->getErrorsArray();
        errorLog.error("Error by parsing transaction");
        errorLog.error("Transaction as base64: %s", DataTypeConverter::binToBase64(binString));
        for(auto it = errors.begin(); it != errors.end();  it++) {
            errorLog.error(*it);
        }
        
    } else {		
        Poco::Logger& transactionLog = LoggerManager::getInstance()->mTransactionLog;
        transactionLog.information("%d %d %s %s\n%s", 
            (int)mMilestoneIndex, (int)mTimestamp, dataIndex.second, mMessageId.toHex(),
            transaction->getJson()
        );
        // if simple validation already failed, we can stop here
        if (!transaction->validate(model::TRANSACTION_VALIDATION_SINGLE)) {
			auto errors = transaction->getErrorsArray();
			errorLog.error("Error by validating transaction");
			for (auto it = errors.begin(); it != errors.end(); it++) {
				errorLog.error(*it);
			}
            return 0;
        }
        // if it is a cross group transaction we store both in Ordering Manager for easy access for validation
        auto transactionBody = transaction->getTransactionBody();
		if (transactionBody->isTransfer() && transactionBody->getTransfer()->isCrossGroupTransfer()) {
			OrderingManager::getInstance()->pushPairedTransaction(transaction);
            // we need also the other pair
            auto pairGroupAlias = transactionBody->getTransfer()->getOtherGroup();
            auto pairGroup = gm->findGroup(pairGroupAlias);
            // we usually not listen on this group so we must do it temporally for validation
            if (pairGroup.isNull()) {
                OrderingManager::getInstance()->checkExternGroupForPairedTransactions(pairGroupAlias);
            }
		}
        // check if transaction already exist
        // if this transaction doesn't belong to us, we can quit here 
        // also if we already have this transaction
        if (group.isNull() || group->isSignatureInCache(transaction)) {
            printf("[%s] transaction skipped because it cames from other group or was found in cache, messageId: %s\n", __FUNCTION__, mMessageId.toHex().data());
            return 0;
        }       
        auto lastTransaction = group->getLastTransaction();
        if (lastTransaction && lastTransaction->getReceivedSeconds() > mTimestamp) {
            // this transaction seems to be from the past, a transaction which happen after this was already added
            printf("[%s] transaction skipped because it cames from the past, messageId: %s\n", __FUNCTION__, mMessageId.toHex().data());
            return 0;
        }
        
        // hand over to OrderingManager
        //std::clog << "transaction: " << std::endl << transaction->getJson() << std::endl;
        OrderingManager::getInstance()->pushTransaction(transaction, mMilestoneIndex, mTimestamp);
    }

    return 0;
}

std::string IotaMessageToTransactionTask::getGradidoGroupAlias(const std::string& iotaIndex) const
{
	auto findPos = iotaIndex.find("GRADIDO");

	if (findPos != iotaIndex.npos) {
		return iotaIndex.substr(8);
	}
	return "";
}
