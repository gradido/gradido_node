#ifndef __GRADIDO_NODE_CONTROLLER_BLOCK_H
#define __GRADIDO_NODE_CONTROLLER_BLOCK_H

#include <map>
#include "ControllerBase.h"
#include "BlockIndex.h"

#include "Poco/AccessExpireCache.h"

//#include "../SingletonManager/FileLockManager.h"

namespace controller {
	class Block : public ControllerBase
	{
	public:
		Block(uint64_t firstTransactionIndex);
		~Block();

		void pushTransaction(const std::string& serializedTransaction, uint64_t index);
		int32_t getFileCursor(uint64_t index);
			
	protected:
		struct TransactionEntry
		{
			TransactionEntry()
				: index(0), fileCursor(0) {}

			uint64_t index;
			std::string serializedTransaction;
			uint32_t fileCursor;
		};

		void updateFileCursors();
		bool calculateFileCursor(TransactionEntry* previousEntry, TransactionEntry* currentEntry);

		uint64_t mFirstTransactionIndex;

		Poco::AccessExpireCache<uint64_t, Poco::SharedPtr<TransactionEntry>> mSerializedTransactions;

		BlockIndex mBlockIndex;
	};
}

#endif