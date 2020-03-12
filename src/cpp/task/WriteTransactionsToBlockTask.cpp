#include "WriteTransactionsToBlockTask.h"
#include "../ServerGlobals.h"

WriteTransactionsToBlockTask::WriteTransactionsToBlockTask(Poco::AutoPtr<model::files::Block> blockFile)
	: UniLib::controller::CPUTask(ServerGlobals::g_WriteFileCPUScheduler), mBlockFile(blockFile)
{

}

WriteTransactionsToBlockTask::~WriteTransactionsToBlockTask()
{
	
}

int WriteTransactionsToBlockTask::run()
{
	Poco::FastMutex::ScopedLock lock(mFastMutex);
	// sort in ascending order by transaction nr
	mTransactions.sort();
	std::vector<std::string> lines;
	lines.reserve(mTransactions.size());
	int lastTransactionNr = 0;
	for (auto it = mTransactions.begin(); it != mTransactions.end(); it++) {
		auto transactionEntry = *it;
		lines.push_back(transactionEntry->serializedTransaction);
		if (lastTransactionNr > 0 && lastTransactionNr + 1 != transactionEntry->transactionNr) {
			addError(new Error(__FUNCTION__, "out of order transaction"));
		}
		lastTransactionNr = transactionEntry->transactionNr;
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
		
		if (result[cursor] != (*it)->getFileCursor()) {
			addError(new Error(__FUNCTION__, "error, calculated file cursor didn't match real file cursor"));
			addError(new ParamError(__FUNCTION__, "transaction nr ", (*it)->transactionNr));
			(*it)->setFileCursor(result[cursor]);
		}
		cursor++;
	}

	return 0;
}