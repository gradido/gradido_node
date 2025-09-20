#include "SimpleOrderingManager.h"
#include "../blockchain/FileBased.h"
#include "../blockchain/FileBasedProvider.h"
#include "../blockchain/Exceptions.h"
#include "../task/HieroMessageToTransactionTask.h"
#include "gradido_blockchain/interaction/toJson/Context.h"
#include "gradido_blockchain/interaction/serialize/Context.h"
#include "gradido_blockchain/interaction/deserialize/Context.h"
#include "gradido_blockchain/const.h"

#include "loguru/loguru.hpp"

#include <stdexcept>

using namespace gradido;
using namespace blockchain;
using namespace interaction;

namespace controller {

    SimpleOrderingManager::SimpleOrderingManager(std::string_view communityId)
        : task::Thread("SimpleOrderingManager"), 
        mLastTransactions(MAGIC_NUMBER_MAX_TIMESPAN_BETWEEN_CREATING_AND_RECEIVING_TRANSACTION * 2), 
        mCommunityId(communityId), 
        mLastSequenceNumber(0)
    {
    }

    SimpleOrderingManager::~SimpleOrderingManager()
    {
    }

    void SimpleOrderingManager::init(uint64_t lastKnownSequenceNumber) 
    {
        mLastSequenceNumber = lastKnownSequenceNumber;
        Thread::init();
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
            auto lastSequenceNumber = getLastSequenceNumber();
            if (lastSequenceNumber + 1 < currentSequenceNumber) {
                // the transaction before is missing so we wait
                LOG_F(2, "transactions arrived out of order from hiero, communityId: %s, last sequence number: %llu, current sequence number: %llu",
                    mCommunityId.data(), lastSequenceNumber, currentSequenceNumber
                );
                return 0;
            }
            // only true if lastSequenceNumber is at least 1 and therefore already set once
            if (lastSequenceNumber && lastSequenceNumber + 1 > currentSequenceNumber) {
                // the transaction after this transaction was already put into blockchain
                // maybe we have an error
                // or hiero has used the same sequence number twice?
                LOG_F(ERROR, "transaction after this was already put into blockchain, fatal error, programm code must be fixed, communityId: %s, last sequence number: %llu, current sequence number: %llu",
                    mCommunityId.data(), lastSequenceNumber, currentSequenceNumber
                );
                it = mTransactions.erase(it);
                return 0;
                // throw GradidoNodeInvalidDataException("data set wasn't like expected, seem to be an error in code or hiero work not like expected");
            }
            auto gradidoTransaction = it->second.deserializeTask->getGradidoTransaction();
            auto& task = it->second.deserializeTask;
            auto body = gradidoTransaction->getTransactionBody();
            // on cross group transaction inbound check if we can found outbound on this gradido node
            // TODO: ask other gradido node(s) if we don't capture the otherGroup (community)
            if (gradido::data::CrossGroupType::INBOUND == body->getType()) {
                auto blockchainProvider = gradido::blockchain::FileBasedProvider::getInstance();
                auto blockchain = blockchainProvider->findBlockchain(mCommunityId);
                Timepoint now = std::chrono::system_clock::now();

                if (it->second.putIntoListTime + MAGIC_NUMBER_MAX_TIMESPAN_BETWEEN_CREATING_AND_RECEIVING_TRANSACTION < now) {
                    // timeouted                    
                    task->notificateFailedTransaction(blockchain, "Transaction skipped (pairing not found)");
                    mTransactions.erase(it);
                    continue;
                }
                auto otherBlockchain = blockchainProvider->findBlockchain(body->getOtherGroup());
                if (!otherBlockchain) {
                    task->notificateFailedTransaction(blockchain, "Transaction skipped (target community unknown)");
                    mTransactions.erase(it);
                    continue;
                }
                deserialize::Context topicIdDeserializer(gradidoTransaction->getParingMessageId(), deserialize::Type::HIERO_TRANSACTION_ID);
                topicIdDeserializer.run();
                if (!topicIdDeserializer.isHieroTransactionId()) {
                    task->notificateFailedTransaction(blockchain, "Transaction skipped (pairing transactionId invalid)");
                    mTransactions.erase(it);
                    continue;
                }
                auto pairTask = static_cast<gradido::blockchain::FileBased*>(otherBlockchain.get())
                    ->getOrderingManager()
                    ->findCrossGroupTransactionPair(topicIdDeserializer.getHieroTransactionId())
                ;
                if (!pairTask || !pairTask->isTaskFinished()) {
                    // not found? maybe it need some more time?
                    std::this_thread::sleep_for(std::chrono::milliseconds(250));
                    continue;
                }
                if (!pairTask->getGradidoTransaction()->isPairing(*gradidoTransaction)) {
                    task->notificateFailedTransaction(blockchain, "Transaction skipped (pairing invalid)");
                    mTransactions.erase(it);
                    continue;
                }
            }
            updateSequenceNumber(lastSequenceNumber + 1);
            if (task->isSuccess()) {
                processTransaction(it->second);
            }            
            mTransactions.erase(it);
            transactionsCount = mTransactions.size();
        } while (transactionsCount);
        return 0;
    }

    void SimpleOrderingManager::processTransaction(const TopicResponseDeserializer& gradidoTransactionWorkData)
    {
        auto blockchainProvider = FileBasedProvider::getInstance();
        auto blockchain = blockchainProvider->findBlockchain(mCommunityId);
        if (!blockchain) {
            throw CommunityNotFoundExceptions("couldn't find group", mCommunityId);
        }
        auto transaction = gradidoTransactionWorkData.deserializeTask->getGradidoTransaction();
        const auto& transactionId = gradidoTransactionWorkData.consensusTopicResponse.getChunkInfo().getInitialTransactionId();
        if (transactionId.empty()) {
            throw GradidoNodeInvalidDataException("missing transaction id in hiero response");
        }
        auto fileBasedBlockchain = std::dynamic_pointer_cast<FileBased>(blockchain);
        try {
            serialize::Context serializer(transactionId);
            bool result = blockchain->createAndAddConfirmedTransaction(
                transaction,
                serializer.run(),
                gradidoTransactionWorkData.consensusTopicResponse.getConsensusTimestamp().getAsTimepoint()
            );
            fileBasedBlockchain->updateLastKnownSequenceNumber(mLastSequenceNumber);
            LOG_F(INFO, "Transaction confirmed, msgId: %s", transactionId.toString().data());
        }
        catch (GradidoBlockchainException& ex) {
            auto communityServer = fileBasedBlockchain->getListeningCommunityServer();
            if (communityServer) {
                communityServer->notificateFailedTransaction(*transaction, ex.what(), transactionId.toString());
            }
            try {
                toJson::Context toJson(*transaction);
                LOG_F(INFO, "transaction not added:\n%s\n%s", ex.getFullString().data(), toJson.run(true).data());
                fileBasedBlockchain->updateLastKnownSequenceNumber(mLastSequenceNumber);
            }
            catch (GradidoBlockchainException& ex) {
                LOG_F(ERROR, "gradido blockchain exception on parsing transaction\n%s", ex.getFullString().data());
            }
            catch (std::exception& ex) {
                LOG_F(ERROR, "exception on parsing transaction:\n%s", ex.what());
            }
        }
    }

    SimpleOrderingManager::PushResult SimpleOrderingManager::pushTransaction(hiero::ConsensusTopicResponse&& consensusTopicResponse) {
        if (isExitCalled()) { return PushResult::IN_SHUTDOWN; }

        std::lock_guard _lock(mTransactionsMutex);
        auto consensusTimestamp = consensusTopicResponse.getConsensusTimestamp();
        const auto& transactionId = consensusTopicResponse.getChunkInfo().getInitialTransactionId();

        auto existingTransactionConsensusTimestamp = mLastTransactions.get(SignatureOctet(*consensusTopicResponse.getRunningHash()));
        if (existingTransactionConsensusTimestamp) {
            if (existingTransactionConsensusTimestamp.value() == consensusTimestamp) {
                LOG_F(2, "skip: %s", transactionId.toString().data());
                return PushResult::FOUND_IN_LAST_TRANSACTIONS;
            }
        }
        
        auto range = mTransactions.equal_range(consensusTimestamp);
        for (auto& it = range.first; it != range.second; ++it) {
            if (it->second.consensusTopicResponse.isMessageSame(consensusTopicResponse)) {
                LOG_F(2, "unexpected skip: %s", transactionId.toString().data());
                return PushResult::FOUND_IN_TRANSACTIONS;
            }
        }
        auto task = std::make_shared<task::HieroMessageToTransactionTask>(consensusTimestamp, consensusTopicResponse.getMessageData(), mCommunityId);
        mLastTransactions.add(SignatureOctet(*consensusTopicResponse.getRunningHash()), consensusTimestamp);
        mTransactions.insert({ consensusTimestamp, TopicResponseDeserializer(std::move(consensusTopicResponse), task) });

        task->scheduleTask(task);
        if (loguru::g_stderr_verbosity >= 0) {
            LOG_F(INFO, "push: %s", transactionId.toString().data());
        }
        return PushResult::ADDED;
    }

    std::shared_ptr<task::HieroMessageToTransactionTask> SimpleOrderingManager::findCrossGroupTransactionPair(const hiero::TransactionId& transactionId) const
    {
        if (isExitCalled()) { return nullptr; }
        std::lock_guard _lock(mTransactionsMutex);
        for (auto it = mTransactions.begin(); it != mTransactions.end(); it++) {
            if (it->second.consensusTopicResponse.getChunkInfo().getInitialTransactionId() == transactionId) {
                return it->second.deserializeTask;
            }
        }
        return nullptr;
    }

    void SimpleOrderingManager::updateSequenceNumber(uint64_t newSequenceNumber)
    {
        
        if (!mLastSequenceNumber) {
            mLastSequenceNumber = newSequenceNumber;
        }
        else 
        {
            if (mLastSequenceNumber + 1 == newSequenceNumber || 1 == newSequenceNumber) {
                mLastSequenceNumber = newSequenceNumber;
                if (1 == newSequenceNumber) {
                    LOG_F(INFO, "reset sequence number for community: %s, topic reset?", mCommunityId.data());
                }
            }
            else {
                throw GradidoNodeInvalidDataException("invalid value for newSequenceNumber");
            }
        }
        LOG_F(2, "new sequenceNumber: %d", mLastSequenceNumber);
    }
}