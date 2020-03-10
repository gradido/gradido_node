#ifndef __GRADIDO_NODE_CONTROLLER_BLOCK_H
#define __GRADIDO_NODE_CONTROLLER_BLOCK_H

#include <map>
#include "ControllerBase.h"
#include "BlockIndex.h"
#include "../model/files/Block.h"

#include "Poco/AccessExpireCache.h"

//#include "../SingletonManager/FileLockManager.h"

namespace controller {
	class Block : public ControllerBase
	{
	public:
		Block(uint32_t blockNr, uint64_t firstTransactionIndex, Poco::Path groupFolderPath);
		~Block();

		bool pushTransaction(const std::string& serializedTransaction, uint64_t transactionNr);
		int32_t getFileCursor(uint64_t transactionNr);
			
	protected:
		struct TransactionEntry
		{
			TransactionEntry()
				: transactionNr(0), fileCursor(0) {}

			TransactionEntry(uint64_t _transactionNr, std::string _serializedTransaction)
				: transactionNr(_transactionNr), serializedTransaction(_serializedTransaction), fileCursor(0) {}

			uint64_t transactionNr;
			std::string serializedTransaction;
			uint32_t fileCursor;
		};

		void updateFileCursors();
		bool calculateFileCursor(TransactionEntry* previousEntry, TransactionEntry* currentEntry);

		uint32_t mBlockNr;
		uint64_t mFirstTransactionIndex;
		int64_t mKtoIndexLowest;
		int64_t mKtoIndexHighest;
		

		Poco::AccessExpireCache<uint64_t, Poco::SharedPtr<TransactionEntry>> mSerializedTransactions;

		BlockIndex mBlockIndex;
		model::files::Block mBlockFile;
	};
}

#endif