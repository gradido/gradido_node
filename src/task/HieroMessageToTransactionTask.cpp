#include "HieroMessageToTransactionTask.h"

#include "gradido_blockchain/lib/Profiler.h"

#include "../SingletonManager/OrderingManager.h"
#include "../blockchain/FileBasedProvider.h"
#include "gradido_blockchain/blockchain/Filter.h"

#include "gradido_blockchain/lib/DataTypeConverter.h"
#include "gradido_blockchain/interaction/toJson/Context.h"
#include "gradido_blockchain/interaction/validate/Context.h"
#include "gradido_blockchain/interaction/deserialize/Context.h"
#include "gradido_blockchain/const.h"
#include "../ServerGlobals.h"

#include "loguru/loguru.hpp"
#include <thread>
#include <chrono>

using namespace gradido;
using namespace data;
using namespace blockchain;
using namespace interaction;
using namespace std::chrono_literals;

HieroMessageToTransactionTask::HieroMessageToTransactionTask(
    Timestamp consensusTimestamp,
    std::shared_ptr<const gradido::data::GradidoTransaction> transaction,
    const std::string& communityId
) : mConsensusTimestamp(consensusTimestamp), mTransaction(transaction), mCommunityId(communityId)
{
#ifdef _UNI_LIB_DEBUG
    setName(DataTypeConverter::timePointToString(consensusTimestamp.getAsTimepoint()).data());
#endif
 
}

HieroMessageToTransactionTask::~HieroMessageToTransactionTask()
{
}

int HieroMessageToTransactionTask::run()
{
    
    LOG_F(1, "start processing transaction: %s", DataTypeConverter::timePointToString(mConsensusTimestamp.getAsTimepoint()).data());
    
    auto blockchainProvider = gradido::blockchain::FileBasedProvider::getInstance();
    auto blockchain = blockchainProvider->findBlockchain(mCommunityId);
    
    if(!blockchain) {
        LOG_F(INFO, "Transaction skipped (unknown community), communityId: %s", mCommunityId.data());
        return 0;
    }

    // log transaction in json format with low verbosity level 1 = debug
    if (loguru::g_stderr_verbosity >= 2) {
        toJson::Context toJson(*mTransaction);
        auto transactionAsJson = toJson.run(true);
        LOG_F(
            2,
            "%s\n%s",
            DataTypeConverter::timePointToString(mConsensusTimestamp).data(),
            transactionAsJson.data()
        );
    }

    // check if transaction already exist
    // if this transaction doesn't belong to us, we can quit here 
    // also if we already have this transaction
    auto fileBasedBlockchain = std::dynamic_pointer_cast<FileBased>(blockchain);
    assert(fileBasedBlockchain);
    if (fileBasedBlockchain->isTransactionExist(mTransaction)) {
        LOG_F(INFO, "Transaction skipped (cached): %s",
            DataTypeConverter::timePointToString(mConsensusTimestamp).data()
        );
        return 0;
    }    
    
    // if simple validation already failed, we can stop here
    try {
        validate::Context validator(*mTransaction);
        validator.run(validate::Type::SINGLE);
    } catch(GradidoBlockchainException& e) {
        LOG_F(ERROR, e.getFullString().data());
        notificateFailedTransaction(blockchain, *mTransaction, e.what());
        return 0;
    }
    
   
    // TODO: Cross Group Check
    // on inbound
    // check if we listen to other group 
    // try to find the pairing transaction 
    if (CrossGroupType::INBOUND == mTransaction->getTransactionBody()->getType()) {
        ConstGradidoTransactionPtr pairingTransaction;
        do {            
            auto otherGroup = blockchainProvider->findBlockchain(mTransaction->getTransactionBody()->getOtherGroup());
            if (otherGroup) {
                interaction::deserialize::Context c(mTransaction->getParingMessageId(), interaction::deserialize::Type::HIERO_TRANSACTION_ID);
                if (c.isHieroTransactionId()) {
                    auto transactionId = c.getHieroTransactionId();
                    Filter f;
                    f.timepointInterval = TimepointInterval(transactionId.getTransactionValidStart(), Timepoint());
                    f.searchDirection = SearchDirection::ASC;
                    f.filterFunction = [&](const TransactionEntry& entry) -> FilterResult {
                        if (entry.getConfirmedTransaction()->getGradidoTransaction()->isPairing(*mTransaction)) {
                            return FilterResult::USE | FilterResult::STOP;
                        }
                        return FilterResult::DISMISS;
                    };
                    auto transactionEntry = otherGroup->findOne(f);
                    if (transactionEntry) {
                        pairingTransaction = transactionEntry->getConfirmedTransaction()->getGradidoTransaction();
                    }
                }
            }
            std::this_thread::sleep_for(100ms);
            // try to find pairing transaction until max time for consensus after consensus timestamp from INBOUND transaction
        } while (!pairingTransaction && mConsensusTimestamp.getAsTimepoint() + MAGIC_NUMBER_MAX_TIMESPAN_BETWEEN_CREATING_AND_RECEIVING_TRANSACTION >= Timepoint());

        if (!pairingTransaction) 
        {
            std::string message = "Transaction skipped (pairing not found)";
            notificateFailedTransaction(blockchain, *mTransaction, message);
            LOG_F(INFO, "%s: %s", message.data(), DataTypeConverter::timePointToString(mConsensusTimestamp).data());
            return 0;
        }
    }
   
    auto lastTransaction = blockchain->findOne(Filter::LAST_TRANSACTION);
    if (lastTransaction && lastTransaction->getConfirmedTransaction()->getConfirmedAt().getAsTimepoint() > mConsensusTimestamp.getAsTimepoint()) {
        // this transaction seems to be from the past, a transaction which happen after this was already added
        std::string message = "Transaction skipped (from past)";
        notificateFailedTransaction(blockchain, *mTransaction, message);
        LOG_F(INFO, "%s: %s", message.data(), DataTypeConverter::timePointToString(mConsensusTimestamp).data());
        return 0;
    }

    // hand over to OrderingManager
    //std::clog << "transaction: " << std::endl << transaction->getJson() << std::endl;
    // OrderingManager::getInstance()->pushTransaction(mTransaction, mConsensusTimestamp, mCommunityId);
    blockchain->createAndAddConfirmedTransaction(mTransaction, std::make_shared<memory::Block>(0), mConsensusTimestamp.getAsTimepoint());

    return 0;
}

void HieroMessageToTransactionTask::notificateFailedTransaction(
    std::shared_ptr<gradido::blockchain::Abstract> blockchain,
	const gradido::data::GradidoTransaction transaction,
	const std::string& errorMessage
)
{
	if (blockchain) {
		auto communityServer = std::dynamic_pointer_cast<gradido::blockchain::FileBased>(blockchain)->getListeningCommunityServer();
		if (communityServer) {
			communityServer->notificateFailedTransaction(transaction, errorMessage, DataTypeConverter::timePointToString(mConsensusTimestamp));
		}
	}
}
