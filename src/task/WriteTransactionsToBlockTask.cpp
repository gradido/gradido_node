#include "WriteTransactionsToBlockTask.h"
#include "../ServerGlobals.h"
#include "gradido_blockchain/GradidoBlockchainException.h"

WriteTransactionsToBlockTask::WriteTransactionsToBlockTask(Poco::AutoPtr<model::files::Block> blockFile, Poco::SharedPtr<controller::BlockIndex> blockIndex)
	: task::CPUTask(ServerGlobals::g_WriteFileCPUScheduler), mBlockFile(blockFile), mBlockIndex(blockIndex)
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
	std::vector<const std::string*> lines;
	lines.reserve(mTransactions.size());

	int lastTransactionNr = 0;

	for (auto it = mTransactions.begin(); it != mTransactions.end(); it++) {
		auto transactionEntry = *it;
		lines.push_back(transactionEntry->getSerializedTransaction());
		if (lastTransactionNr > 0 && lastTransactionNr + 1 != transactionEntry->getTransactionNr()) {
			throw BlockchainOrderException("out of order transaction");
		}
		lastTransactionNr = transactionEntry->getTransactionNr();
	}

	auto resultingCursorPositions = mBlockFile->appendLines(lines);
	
	size_t cursor = 0;
	for (auto it = mTransactions.begin(); it != mTransactions.end(); it++) {
		(*it)->setFileCursor(resultingCursorPositions[cursor]);
		mBlockIndex->addFileCursorForTransaction((*it)->getTransactionNr(), resultingCursorPositions[cursor]);
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

Poco::SharedPtr<model::NodeTransactionEntry> WriteTransactionsToBlockTask::getTransaction(uint64_t nr)
{
	for (auto it = mTransactions.begin(); it != mTransactions.end(); it++) {
		if ((*it)->getTransactionNr() == nr) {
			return *it;
		}
	}
	return nullptr;
}