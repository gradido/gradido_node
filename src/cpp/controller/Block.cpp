#include "Block.h"
#include "../ServerGlobals.h"

namespace controller {
	Block::Block(uint64_t firstTransactionIndex)
		: mFirstTransactionIndex(firstTransactionIndex), mSerializedTransactions(ServerGlobals::g_CacheTimeout)
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

	void Block::pushTransaction(const std::string& serializedTransaction, uint64_t index) 
	{
		Poco::FastMutex::ScopedLock lock(mWorkingMutex);

		auto entry = new TransactionEntry;
		entry->index = index;
		entry->serializedTransaction = serializedTransaction;


		auto insertPair = mSerializedTransactions.insert(std::pair<uint64_t, TransactionEntry*>(index, entry));
		if (insertPair.second) {
			auto prevEntryIt = insertPair.first--;
			calculateFileCursor(prevEntryIt->second, entry);
		}
	}

	int32_t Block::getFileCursor(uint64_t index)
	{
		Poco::FastMutex::ScopedLock lock(mWorkingMutex);

		auto it = mSerializedTransactions.find(index);
		if (it == mSerializedTransactions.end()) {
			return -1;
		}
		auto entry = it->second;
		if (entry->fileCursor || it->first == mFirstTransactionIndex) {
			return entry->fileCursor;
		}
		updateFileCursors();
		if (entry->fileCursor || it->first == mFirstTransactionIndex) {
			return entry->fileCursor;
		}
		return -2;
	}

	void Block::updateFileCursors()
	{
		TransactionEntry* prevEntry = nullptr;
		for (auto it = mSerializedTransactions.begin(); it != mSerializedTransactions.end(); it++) {
			if (prevEntry) {
				if (!calculateFileCursor(prevEntry, it->second)) {
					break;
				}
			}
			prevEntry = it->second;
		}
	}

	bool Block::calculateFileCursor(TransactionEntry* previousEntry, TransactionEntry* currentEntry)
	{
		if (previousEntry->fileCursor || previousEntry->index == mFirstTransactionIndex) {
			currentEntry->fileCursor = previousEntry->fileCursor + 2 + previousEntry->serializedTransaction.size();
			return true;
		}
		return false;
	}
}