#include "ArchiveTransactionsOrdering.h"
#include "../SingletonManager/LoggerManager.h"
#include "gradido_blockchain/MemoryManager.h"
#include "gradido_blockchain/lib/Profiler.h"
#include "Group.h"
#include "Poco/Util/ServerApplication.h"

namespace controller {
	ArchiveTransactionsOrdering::ArchiveTransactionsOrdering(Group* parentGroup)
		: Thread("grdArchiveOrd"),  mParentGroup(parentGroup)
	{

	}

	ArchiveTransactionsOrdering::~ArchiveTransactionsOrdering()
	{

	}

	void ArchiveTransactionsOrdering::addPendingTransaction(
		std::unique_ptr<model::gradido::GradidoTransaction> transaction,
		uint64_t transactionNr
	)
	{
		auto nextTransactionId = getNextTransactionId();
		if (nextTransactionId > transactionNr) {
			throw ArchiveTransactionDoubletteException("transaction with this nr already exist", transactionNr);
		}
		if (nextTransactionId == transactionNr) {
			insertTransactionToGroup(std::move(transaction));
			printf("\radded transaction: %d/%d", transactionNr, mPendingTransactions.size());
		}
		else {
			auto lastTransaction = mParentGroup->getLastTransaction();
			if (!lastTransaction.isNull() && lastTransaction->getReceived() > transaction->getTransactionBody()->getCreatedSeconds()) {
				throw BlockchainOrderException("previous transaction is younger");
			}
			std::scoped_lock<std::shared_mutex> _lock(mPendingTransactionsMutex);
			// prevent that hackers can fill up the memory with pending archive transactions
			// MAGIC NUMBER
			if (mPendingTransactions.size() > 6000) {
				throw ArchivePendingTransactionsMapFull("archive pending map exhausted", mPendingTransactions.size());
			}
			auto result = mPendingTransactions.insert({ transactionNr, std::move(transaction) });
			if (!result.second) {
				throw ArchiveTransactionDoubletteException("transaction with this nr already exist", transactionNr);
			}
			condSignal();
		}
		
	}

	int ArchiveTransactionsOrdering::ThreadFunction()
	{
		PendingTransactionsMap::iterator it;
		while(true)
		{		
			uint64_t transactionNr = 0;
			std::unique_ptr<model::gradido::GradidoTransaction> gradidoTransaction;
			{
				std::scoped_lock<std::shared_mutex> _lock(mPendingTransactionsMutex);
				
				if (!mPendingTransactions.size()) {
					return 0;
				}
				it = mPendingTransactions.begin();

				auto nextTransactionId = getNextTransactionId();
				if (it->first != nextTransactionId) {
					return 0;
				}
				gradidoTransaction = std::move(it->second);
				transactionNr = it->first;
			}		
			
			try {
				insertTransactionToGroup(std::move(gradidoTransaction));
			}
			catch (GradidoBlockchainException& ex) {
				LoggerManager::getInstance()->mErrorLogging.warning("[ArchiveTransactionsOrdering] terminate with gradido blockchain exception: %s", ex.getFullString());
			}
			catch (Poco::Exception& ex) {
				LoggerManager::getInstance()->mErrorLogging.critical("[ArchiveTransactionsOrdering] terminate with poco exception: %s", ex.displayText());
				Poco::Util::ServerApplication::terminate();
			}
			catch (std::exception& ex) {
				LoggerManager::getInstance()->mErrorLogging.critical("[ArchiveTransactionsOrdering] terminate with exception: %s", std::string(ex.what()));
				Poco::Util::ServerApplication::terminate();
			}
			{
				std::scoped_lock<std::shared_mutex> _lock(mPendingTransactionsMutex);
				it = mPendingTransactions.find(transactionNr);
				assert(it != mPendingTransactions.end());
				mPendingTransactions.erase(it);
				printf("\radded transaction: %d/%d", transactionNr, mPendingTransactions.size() + transactionNr);
			}
		}

		return 0;
	}

	uint64_t ArchiveTransactionsOrdering::getNextTransactionId()
	{
		auto lastTransaction = mParentGroup->getLastTransaction();
		if (!lastTransaction.isNull()) {
			return lastTransaction->getID() + 1;
		}
		else {
			return 1;
		}
	}

	void ArchiveTransactionsOrdering::insertTransactionToGroup(std::unique_ptr<model::gradido::GradidoTransaction> transaction)
	{
		auto createdTimestamp = transaction->getTransactionBody()->getCreatedSeconds();
		// calculate hash with blake2b
		auto mm = MemoryManager::getInstance();
		auto hash = mm->getMemory(crypto_generichash_BYTES);
		auto rawMessage = transaction->getSerialized();

		crypto_generichash(
			*hash, hash->size(),
			(const unsigned char*)rawMessage->data(),
			rawMessage->size(),
			NULL, 0
		);
		try {
			Profiler timeAddingTransaction;
			auto type = transaction->getTransactionBody()->getTransactionType();
			mParentGroup->addTransaction(std::move(transaction), hash, createdTimestamp);
			LoggerManager::getInstance()->mSpeedLogging.information(
				"time adding archived transaction: %s of type: %s", 
				timeAddingTransaction.string(), std::string(model::gradido::TransactionBody::transactionTypeToString(type)));
		}
		catch (Poco::NullPointerException& ex) {
			printf("poco null pointer exception by calling Group::addTransaction\n");
			throw;
		}
		mm->releaseMemory(hash);
	}

	// *************** Exceptions ****************************
	ArchiveTransactionDoubletteException::ArchiveTransactionDoubletteException(const char* what, uint64_t transactionNr) noexcept
		: GradidoBlockchainException(what), mTransactionNr(transactionNr)
	{


	}

	std::string ArchiveTransactionDoubletteException::getFullString() const
	{
		std::string result = what();
		result += ", transaction nr: " + std::to_string(mTransactionNr);
		return result;
	}

	ArchivePendingTransactionsMapFull::ArchivePendingTransactionsMapFull(const char* what, size_t mapSize) noexcept
		: GradidoBlockchainException(what), mMapSize(mapSize)
	{

	}

	std::string ArchivePendingTransactionsMapFull::getFullString() const
	{
		std::string result = what();
		result += ", map size: " + std::to_string(mMapSize);
		return result;
	}
}
