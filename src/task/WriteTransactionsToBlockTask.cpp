#include "WriteTransactionsToBlockTask.h"
#include "../blockchain/NodeTransactionEntry.h"
#include "../cache/BlockIndex.h"
#include "../model/files/Block.h"
#include "../ServerGlobals.h"

#include "gradido_blockchain/GradidoBlockchainException.h"

#include "loguru/loguru.hpp"
#include <cassert>

WriteTransactionsToBlockTask::WriteTransactionsToBlockTask(std::shared_ptr<model::files::Block> blockFile, std::shared_ptr<cache::BlockIndex> blockIndex)
	: task::CPUTask(ServerGlobals::g_WriteFileCPUScheduler), mBlockFile(blockFile), mBlockIndex(blockIndex)
{
	assert(blockFile);
	assert(blockIndex);
}

WriteTransactionsToBlockTask::~WriteTransactionsToBlockTask()
{
}

int WriteTransactionsToBlockTask::run()
{
	Poco::FastMutex::ScopedLock lock(mFastMutex);
	std::vector<memory::ConstBlockPtr> lines;
	lines.reserve(mTransactions.size());

	int lastTransactionNr = 0;

	for (auto it = mTransactions.begin(); it != mTransactions.end(); ++it) {
		auto transactionEntry = it->second;
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

	LOG_F(INFO, "[%u;%u] transaction written to block file: %s",
		(unsigned)mTransactions.begin()->first, (unsigned)(mTransactions.begin()->first + cursor),
		mBlockFile->getBlockPath()
	);
	return 0;
}

void WriteTransactionsToBlockTask::addSerializedTransaction(std::shared_ptr<gradido::blockchain::NodeTransactionEntry> transaction)
{
	assert(!isTaskSheduled());
	assert(!isTaskFinished());		
	Poco::FastMutex::ScopedLock lock(mFastMutex);	 
	mTransactions.insert({transaction->getTransactionNr(), transaction});
	mBlockIndex->addIndicesForTransaction(transaction);
}

std::shared_ptr<gradido::blockchain::NodeTransactionEntry> WriteTransactionsToBlockTask::getTransaction(uint64_t nr)
{
	auto it = mTransactions.find(nr);
	if(it == mTransactions.end()) {
		return nullptr;
	}
	return it->second;
}