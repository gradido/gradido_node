#include "SimpleOrderingManager.h"
#include "GlobalStateManager.h"
#include "../blockchain/FileBased.h"
#include "../blockchain/FileBasedProvider.h"
#include "../blockchain/Exceptions.h"
#include "gradido_blockchain/interaction/toJson/Context.h"
#include "gradido_blockchain/interaction/serialize/Context.h"
#include "gradido_blockchain/lib/DataTypeConverter.h"
#include "gradido_blockchain/const.h"

#include "loguru/loguru.hpp"

#include <stdexcept>

using namespace gradido;
using namespace blockchain;
using namespace interaction;

SimpleOrderingManager::SimpleOrderingManager()
    : task::Thread("SimpleOrderingManager"), mLastTransactions(MAGIC_NUMBER_MAX_TIMESPAN_BETWEEN_CREATING_AND_RECEIVING_TRANSACTION * 2)
{
}

SimpleOrderingManager::~SimpleOrderingManager()
{
}

SimpleOrderingManager* SimpleOrderingManager::getInstance()
{
    static SimpleOrderingManager theOne;
    return &theOne;
}


int SimpleOrderingManager::ThreadFunction()
{
    size_t transactionsCount = 0; 
    do {        
        std::unique_lock _lock(mTransactionsMutex);
        auto it = mTransactions.begin();
        // if no transaction is in map or first transaction deserialize task is still running (or is waiting to be scheduled)
        if (it == mTransactions.end() || !it->second.deserializeTask->isTaskFinished()) {
            // with return 0, thread is still running, and this function will called again with next condSignal call
            return 0;
        }
        auto currentSequenceNumber = it->second.consensusTopicResponse.getSequenceNumber();
        auto lastSequenceNumber = getLastSequenceNumber(it->second.communityId);
        if (lastSequenceNumber + 1 < currentSequenceNumber) {
            // the transaction before is missing so we wait
            LOG_F(2, "transactions arrived out of order from hiero, communityId: %s, last sequence number: %llu, current sequence number: %llu",
                it->second.communityId.data(), lastSequenceNumber, currentSequenceNumber
            );
            return 0;
        }
        // only true if lastSequenceNumber is at least 1 and therefore already set once
        if (lastSequenceNumber && lastSequenceNumber + 1 > currentSequenceNumber) {
            // the transaction after this transaction was already put into blockchain
            // maybe we have an error
            // or hiero has used the same sequence number twice?
            LOG_F(FATAL, "transaction after this was already put into blockchain, fatal error, programm code must be fixed, communityId: %s, last sequence number: %llu, current sequence number: %llu",
                it->second.communityId.data(), lastSequenceNumber, currentSequenceNumber
            );
            throw GradidoNodeInvalidDataException("data set wasn't like expected, seem to be an error in code or hiero work not like expected");
        }
        if (it->second.deserializeTask->isSuccess()) {
            processTransaction(it->second);
        }
        updateSequenceNumber(it->second.communityId.data(), lastSequenceNumber + 1);
        mTransactions.erase(it);
        transactionsCount = mTransactions.size();        
    } while (transactionsCount);
    return 0;
}

void SimpleOrderingManager::processTransaction(const GradidoTransactionWithGroup& gradidoTransactionWorkData)
{
    auto blockchainProvider = FileBasedProvider::getInstance();
    auto blockchain = blockchainProvider->findBlockchain(gradidoTransactionWorkData.communityId);
    if (!blockchain) {
        throw CommunityNotFoundExceptions("couldn't find group", gradidoTransactionWorkData.communityId);
    }
    auto transaction = gradidoTransactionWorkData.deserializeTask->getGradidoTransaction();
    const auto& transactionId = gradidoTransactionWorkData.consensusTopicResponse.getChunkInfo().getInitialTransactionId();
    if (!transactionId.empty()) {
        throw GradidoNodeInvalidDataException("missing transaction id in hiero response");
    }
    try {                
        serialize::Context serializer(transactionId);
        bool result = blockchain->createAndAddConfirmedTransaction(
            transaction, 
            serializer.run(),
            gradidoTransactionWorkData.consensusTopicResponse.getConsensusTimestamp().getAsTimepoint()
        );
        LOG_F(INFO, "Transaction added, msgId: %s", transactionId.toString().data());
    }
    catch (GradidoBlockchainException& ex) {
        auto fileBasedBlockchain = std::dynamic_pointer_cast<FileBased>(blockchain);
        auto communityServer = fileBasedBlockchain->getListeningCommunityServer();
        if (communityServer) {
            communityServer->notificateFailedTransaction(*transaction, ex.what(), transactionId.toString());
        }
        try {
            toJson::Context toJson(*transaction);
            LOG_F(INFO, "transaction not added:\n%s\n%s", ex.getFullString().data(), toJson.run(true).data());
        }
        catch (GradidoBlockchainException& ex) {
            LOG_F(ERROR, "gradido blockchain exception on parsing transaction\n%s", ex.getFullString().data());
        }
        catch (std::exception& ex) {
            LOG_F(ERROR, "exception on parsing transaction:\n%s", ex.what());
        }
    }
}

SimpleOrderingManager::PushResult SimpleOrderingManager::pushTransaction(
    hiero::ConsensusTopicResponse&& consensusTopicResponse, const std::string_view communityId
) {
    if (isExitCalled()) { return; }
    auto consensusTimestamp = consensusTopicResponse.getConsensusTimestamp();

    auto existingTransactionConsensusTimestamp = mLastTransactions.get(SignatureOctet(*consensusTopicResponse.getRunningHash()));
    if (existingTransactionConsensusTimestamp) {
        if (existingTransactionConsensusTimestamp.value() == consensusTimestamp) {
            LOG_F(2, "skip: %s", DataTypeConverter::timePointToString(consensusTimestamp.getAsTimepoint()).data());
            return PushResult::FOUND_IN_LAST_TRANSACTIONS;
        }
    }
    std::lock_guard _lock(mTransactionsMutex);
    auto range = mTransactions.equal_range(consensusTimestamp);
    for (auto& it = range.first; it != range.second; ++it) {
        if (it->second.communityId == communityId && it->second.consensusTopicResponse.isMessageSame(consensusTopicResponse)) {
            LOG_F(2, "unexpected skip: %s", DataTypeConverter::timePointToString(consensusTimestamp.getAsTimepoint()).data());
            return PushResult::FOUND_IN_TRANSACTIONS;
        }
    }
    std::shared_ptr<HieroMessageToTransactionTask> task(new HieroMessageToTransactionTask(
        consensusTimestamp, consensusTopicResponse.getMessageData(), communityId
    ));
    mTransactions.insert({ consensusTimestamp, GradidoTransactionWithGroup(std::move(consensusTopicResponse), communityId, task) });
    
    task->scheduleTask(task);
    if (loguru::g_stderr_verbosity >= 0) {
        LOG_F(INFO, "add: %s", DataTypeConverter::timePointToString(consensusTimestamp.getAsTimepoint()).data());
    }
    return PushResult::ADDED;
}


uint64_t SimpleOrderingManager::getLastSequenceNumber(const std::string& communityId) const
{
    auto it = mGroupAliasLastSequenceNumbers.find(communityId);
    if (it != mGroupAliasLastSequenceNumbers.end()) {
        return it->second;
    }
    return 0;
}
void SimpleOrderingManager::updateSequenceNumber(const std::string& communityId, uint64_t newSequenceNumber)
{
    auto it = mGroupAliasLastSequenceNumbers.find(communityId);
    if (it == mGroupAliasLastSequenceNumbers.end()) {
        mGroupAliasLastSequenceNumbers.insert({ communityId, newSequenceNumber });
    }
    else {
        if (it->second + 1 == newSequenceNumber || 1 == newSequenceNumber) {
            it->second = newSequenceNumber;
            if (1 == newSequenceNumber) {
                LOG_F(INFO, "reset sequence number for community: %s, topic reset?", communityId.data());
            }
        }
        else {
            throw GradidoNodeInvalidDataException("invalid value for newSequenceNumber");
        }
    }

}
