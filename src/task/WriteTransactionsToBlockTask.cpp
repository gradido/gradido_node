#include "WriteTransactionsToBlockTask.h"
#include "../ServerGlobals.h"

WriteTransactionsToBlockTask::WriteTransactionsToBlockTask(Poco::AutoPtr<model::files::Block> blockFile, Poco::SharedPtr<controller::BlockIndex> blockIndex)
	: UniLib::controller::CPUTask(ServerGlobals::g_WriteFileCPUScheduler), mBlockFile(blockFile), mBlockIndex(blockIndex)
{
	assert(!blockFile.isNull());
	assert(!blockIndex.isNull());
}

WriteTransactionsToBlockTask::~WriteTransactionsToBlockTask()
{
	
}

int WriteTransactionsToBlockTask::run()
{
	Poco::FastMutex::ScopedLock lock(mFastMutex);
	// sort in ascending order by transaction nr
	mTransactions.sort([](Poco::SharedPtr<model::TransactionEntry> a, Poco::SharedPtr<model::TransactionEntry> b) {
		return a->getTransactionNr() < b->getTransactionNr();
	});
	std::vector<std::string> lines;
	lines.reserve(mTransactions.size());

	int lastTransactionNr = 0;

	for (auto it = mTransactions.begin(); it != mTransactions.end(); it++) {
		auto transactionEntry = *it;
		lines.push_back(transactionEntry->getSerializedTransaction());
		if (lastTransactionNr > 0 && lastTransactionNr + 1 != transactionEntry->getTransactionNr()) {
			addError(new Error(__FUNCTION__, "out of order transaction"));
		}
		lastTransactionNr = transactionEntry->getTransactionNr();
	}

	auto result = mBlockFile->appendLines(lines);
	if (result.size() != mTransactions.size()) {
		addError(new Error(__FUNCTION__, "error by append lines, result vector size didn't match transaction list size"));
	}

	size_t cursor = 0;
	for (auto it = mTransactions.begin(); it != mTransactions.end(); it++) {
		if (cursor >= result.size()) {
			addError(new Error(__FUNCTION__, "not enough results"));
			break;
		}
		
		if (result[cursor] < 0) {
			addError(new ParamError(__FUNCTION__, "critical, error in append line, result", result[cursor]));
		}
		else {
			(*it)->setFileCursor(result[cursor]);
			mBlockIndex->addFileCursorForTransaction((*it)->getTransactionNr(), result[cursor]);
		}
		cursor++;
	}
	// save also block index
	mBlockIndex->writeIntoFile();

	return 0;
}

std::vector<uint64_t> WriteTransactionsToBlockTask::getTransactionNrs()
{
	Poco::FastMutex::ScopedLock lock(mFastMutex);
	std::vector<uint64_t> transactionNrs;
	transactionNrs.reserve(mTransactions.size());
	for (auto it = mTransactions.begin(); it != mTransactions.end(); it++) {
		transactionNrs.push_back((*it)->getTransactionNr());
	}
	return transactionNrs;
}

Poco::SharedPtr<model::TransactionEntry> WriteTransactionsToBlockTask::getTransaction(uint64_t nr)
{
	for (auto it = mTransactions.begin(); it != mTransactions.end(); it++) {
		if ((*it)->getTransactionNr() == nr) {
			return *it;
		}
	}
	return nullptr;
}