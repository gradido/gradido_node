#include "WriteTransactionsToBlockTask.h"
#include "../ServerGlobals.h"
#include "gradido_blockchain/GradidoBlockchainException.h"
#include "../SingletonManager/LoggerManager.h"

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
		auto base64 = DataTypeConverter::binToBase64(*transactionEntry->getSerializedTransaction());
		//printf("serialized transaction: %s\n", base64.data());
		lines.push_back(transactionEntry->getSerializedTransaction());
		if (lastTransactionNr > 0 && lastTransactionNr + 1 != transactionEntry->getTransactionNr()) {
			throw BlockchainOrderException("out of order transaction");
		}
		lastTransactionNr = transactionEntry->getTransactionNr();
	}
	auto resultingCursorPositions = mBlockFile->appendLines(lines);
	assert(resultingCursorPositions.size() == lines.size());
	
	size_t cursor = 0;
	for (auto it = mTransactions.begin(); it != mTransactions.end(); it++) {
		assert(it == mTransactions.begin() || resultingCursorPositions[cursor]);
		(*it)->setFileCursor(resultingCursorPositions[cursor]);
		mBlockIndex->addFileCursorForTransaction((*it)->getTransactionNr(), resultingCursorPositions[cursor]);
		cursor++;
	}
	// save also block index
	mBlockIndex->writeIntoFile();
	auto lastEntry = mTransactions.end();
	lastEntry--;
	LoggerManager::getInstance()->mErrorLogging.debug(
		"write transactions from: %u to %u into file",
		(unsigned)(*mTransactions.begin())->getTransactionNr(),
		(unsigned)(*lastEntry)->getTransactionNr()
	);
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