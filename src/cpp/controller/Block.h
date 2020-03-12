#ifndef __GRADIDO_NODE_CONTROLLER_BLOCK_H
#define __GRADIDO_NODE_CONTROLLER_BLOCK_H

#include <map>
#include "ControllerBase.h"
#include "BlockIndex.h"

#include "../model/files/Block.h"

#include "../task/WriteTransactionsToBlockTask.h"

#include "../SingletonManager/TimeoutManager.h"

#include "Poco/AccessExpireCache.h"

//#include "../SingletonManager/FileLockManager.h"

namespace controller {

	struct TransactionEntry
	{
		TransactionEntry()
			: transactionNr(0), fileCursor(0) {}

		TransactionEntry(uint64_t _transactionNr, std::string _serializedTransaction)
			: transactionNr(_transactionNr), serializedTransaction(_serializedTransaction), fileCursor(0) {}

		bool operator < (const TransactionEntry& b) { return transactionNr < b.transactionNr;}

		inline void setFileCursor(uint32_t newFileCursorValue) { Poco::FastMutex::ScopedLock lock(fastMutex); fileCursor = newFileCursorValue; }
		inline uint32_t getFileCursor() { Poco::FastMutex::ScopedLock lock(fastMutex); return fileCursor; }

		uint64_t transactionNr;
		std::string serializedTransaction;
		uint32_t fileCursor;
		Poco::FastMutex fastMutex;
	};

	class Block : public ControllerBase, public ITimeout
	{
	public:
		Block(uint32_t blockNr, uint64_t firstTransactionIndex, Poco::Path groupFolderPath);
		~Block();

		bool pushTransaction(const std::string& serializedTransaction, uint64_t transactionNr);
		
		int getTransaction(uint64_t transactionNr, std::string& serializedTransaction);

		void checkTimeout();
			
	protected:

		uint32_t mBlockNr;
		uint64_t mFirstTransactionIndex;
		int64_t mKtoIndexLowest;
		int64_t mKtoIndexHighest;

		Poco::AccessExpireCache<uint64_t, TransactionEntry> mSerializedTransactions;

		BlockIndex mBlockIndex;
		Poco::AutoPtr<model::files::Block> mBlockFile;
		Poco::AutoPtr<WriteTransactionsToBlockTask> mTransactionWriteTask;
	};
}

#endif