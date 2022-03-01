#include "IotaMessageToTransactionTask.h"

#include "gradido_blockchain/model/protobufWrapper/GradidoTransaction.h"
#include "gradido_blockchain/model/protobufWrapper/TransactionValidationExceptions.h"

#include "../SingletonManager/LoggerManager.h"
#include "../SingletonManager/GroupManager.h"
#include "../SingletonManager/OrderingManager.h"

#include "gradido_blockchain/lib/DataTypeConverter.h"
#include "gradido_blockchain/http/IotaRequestExceptions.h"
#include "../ServerGlobals.h"

#include "Poco/Util/ServerApplication.h"

//#include "../lib/BinTextConverter.h"


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

// TODO: has thrown a null pointer exception
int IotaMessageToTransactionTask::run()
{
    Poco::Logger& errorLog = LoggerManager::getInstance()->mErrorLogging;
    std::pair<std::string, std::unique_ptr<std::string>> dataIndex;
    try {
        dataIndex = ServerGlobals::g_IotaRequestHandler->getIndexiationMessageDataIndex(mMessageId.toHex());
    }
    catch (...) {
        IotaRequest::defaultExceptionHandler(errorLog, false);
        errorLog.warning("retry once after waiting 100 ms");
        Poco::Thread::sleep(100);
		try {
			dataIndex = ServerGlobals::g_IotaRequestHandler->getIndexiationMessageDataIndex(mMessageId.toHex());
		}
		catch (...) {
            // terminate application
            IotaRequest::defaultExceptionHandler(errorLog, true);
		}
    }

    auto binString = DataTypeConverter::hexToBinString(dataIndex.first);
    auto gm = GroupManager::getInstance();
    auto group = gm->findGroup(getGradidoGroupAlias(*dataIndex.second.get()));
    
    Poco::AutoPtr<model::gradido::GradidoTransaction> transaction(new model::gradido::GradidoTransaction(*binString.get()));
        
    Poco::Logger& transactionLog = LoggerManager::getInstance()->mTransactionLog;
    transactionLog.information("%d %d %s %s\n%s", 
        (int)mMilestoneIndex, (int)mTimestamp, dataIndex.second, mMessageId.toHex(),
        transaction->toJson()
    );
    // if simple validation already failed, we can stop here
    try {
        transaction->validate(model::gradido::TRANSACTION_VALIDATION_SINGLE);
        if (transaction->getTransactionBody()->isCreation()) {
            transaction->getTransactionBody()->getCreationTransaction()->validateTargetDate(mTimestamp);
        }
    } catch(model::gradido::TransactionValidationException& e) {
        errorLog.error(e.getFullString());
        return 0;
    }
    
    // if it is a cross group transaction we store both in Ordering Manager for easy access for validation
    auto transactionBody = transaction->getTransactionBody();
	if (transactionBody->isTransfer() && !transactionBody->isLocal()) {
		OrderingManager::getInstance()->pushPairedTransaction(transaction);
        // we need also the other pair
        auto pairGroupAlias = transactionBody->getOtherGroup();
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
    if (lastTransaction && lastTransaction->getReceived() > mTimestamp) {
        // this transaction seems to be from the past, a transaction which happen after this was already added
        printf("[%s] transaction skipped because it cames from the past, messageId: %s\n", __FUNCTION__, mMessageId.toHex().data());
        return 0;
    }
        
    // hand over to OrderingManager
    //std::clog << "transaction: " << std::endl << transaction->getJson() << std::endl;
    OrderingManager::getInstance()->pushTransaction(transaction, mMilestoneIndex, mTimestamp);

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
