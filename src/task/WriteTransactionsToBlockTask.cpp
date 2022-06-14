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
	std::vector<const std::string*> lines;
	lines.reserve(mTransactions.size());

	int lastTransactionNr = 0;

	for (auto it = mTransactions.begin(); it != mTransactions.end(); ++it) {
		auto transactionEntry = it->second;
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
	uint32_t fileCursor = 0;
	for (auto it = mTransactions.begin(); it != mTransactions.end(); ++it) {
		assert(it == mTransactions.begin() || resultingCursorPositions[cursor]);
		fileCursor = resultingCursorPositions[cursor];
		it->second->setFileCursor(fileCursor);
		mBlockIndex->addFileCursorForTransaction(it->second->getTransactionNr(), fileCursor);
		cursor++;
	}
	// save also block index
	mBlockIndex->writeIntoFile();

	LoggerManager::getInstance()->mErrorLogging.information("[%u;%u] transaction written to block file: %s",
		(unsigned)mTransactions.begin()->first, (unsigned)(mTransactions.begin()->first + cursor), 
		mBlockFile->getBlockPath()
	);
	return 0;
}

void WriteTransactionsToBlockTask::addSerializedTransaction(Poco::SharedPtr<model::NodeTransactionEntry> transaction) 
{
	assert(!isTaskSheduled());
	assert(!isTaskFinished());		
	Poco::FastMutex::ScopedLock lock(mFastMutex);	 
	mTransactions.insert({transaction->getTransactionNr(), transaction});
	mBlockIndex->addIndicesForTransaction(transaction);
}

Poco::SharedPtr<model::NodeTransactionEntry> WriteTransactionsToBlockTask::getTransaction(uint64_t nr)
{
	auto it = mTransactions.find(nr);
	if(it == mTransactions.end()) {
		return nullptr;
	}
	return it->second;
}