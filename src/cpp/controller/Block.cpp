#include "Block.h"
#include "../ServerGlobals.h"

namespace controller {
	Block::Block(uint32_t blockNr, uint64_t firstTransactionIndex, Poco::Path groupFolderPath)
		: mBlockNr(blockNr), mFirstTransactionIndex(firstTransactionIndex), mSerializedTransactions(ServerGlobals::g_CacheTimeout),
		  mBlockFile(new model::files::Block(groupFolderPath, blockNr))
	{

	}

	Block::~Block() 
	{
		Poco::FastMutex::ScopedLock lock(mWorkingMutex);

		/*for (auto it = mSerializedTransactions.begin(); it != mSerializedTransactions.end(); it++) {
			delete it->second;
		}*/
		mSerializedTransactions.clear();
	}

	bool Block::pushTransaction(const std::string& serializedTransaction, uint64_t transactionNr)
	{
		Poco::FastMutex::ScopedLock lock(mWorkingMutex);

		//auto insertPair = mSerializedTransactions.insert(std::pair<uint64_t, TransactionEntry*>(transactionNr, entry));
		auto transactionEntry = new TransactionEntry(transactionNr, serializedTransaction);
		//transactionEntry->fileCursor = getFileCursor(transactionNr);
		transactionEntry->fileCursor = mBlockFile->appendLine(serializedTransaction);
		if (transactionEntry->fileCursor > 0) {
			mSerializedTransactions.add(transactionNr, transactionEntry);
			return true;
		}
		return false;

		/*if (insertPair.second) {
			auto prevEntryIt = insertPair.first--;
			calculateFileCursor(prevEntryIt->second, entry);
		}*/
	}

	int Block::getTransaction(uint64_t transactionNr, std::string& serializedTransaction)
	{
		if (transactionNr == 0) return -1;
		auto transactionEntry = mSerializedTransactions.get(transactionNr);
		if (transactionEntry.isNull()) {
			// read from file system	

		} 
		if (transactionEntry.isNull()) {
			return -1;
		}

		serializedTransaction = transactionEntry->serializedTransaction;
		return 0;
	}

	void Block::checkTimeout()
	{
		Poco::FastMutex::ScopedLock lock(mWorkingMutex);

		if (!mTransactionWriteTask.isNull()) {			
			if (Poco::Timespan(Poco::DateTime() - mTransactionWriteTask->getCreationDate()).totalSeconds() > ServerGlobals::g_WriteToDiskTimeout) {
				mTransactionWriteTask->scheduleTask(mTransactionWriteTask);
				mTransactionWriteTask = nullptr;
			}
		}
	}

}