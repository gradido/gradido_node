#include "RebuildBlockIndexTask.h"
#include "BatchDeserializeConfirmedTransactionTask.h"
#include "../ServerGlobals.h"
#include "../blockchain/FileBased.h"

#include "gradido_blockchain/lib/Profiler.h"
#include "gradido_blockchain/memory/Block.h"
#include "gradido_blockchain/serialization/toJsonString.h"

using memory::Block;
using gradido::blockchain::FileBased;
using std::shared_ptr, std::make_shared;
using ServerGlobals::g_CPUScheduler;

namespace task {
	RebuildBlockIndexTask::RebuildBlockIndexTask(
		std::shared_ptr<const gradido::blockchain::FileBased> blockchain,
		std::shared_ptr<cache::BlockIndex> blockIndex,
		IMutableDictionary<memory::ConstBlockPtr>& publicKeyDictionary
	): task::CPUTask(g_CPUScheduler), mBlockchain(blockchain), mBlockIndex(blockIndex), mPublicKeyIndex(publicKeyDictionary)
	{
		mRawTransactionsBulk.reserve(REBUILD_BLOCK_INDEX_TASK_BULK_SIZE);
		mActiveDeserializerTasks = 0;
	}

	int RebuildBlockIndexTask::run()
	{
		LOG_F(WARNING, "not supposed to sheduled as regular task");
		return 0;
	}

	void RebuildBlockIndexTask::pushLine(int32_t fileCursor, memory::ConstBlockPtr line, std::shared_ptr<RebuildBlockIndexTask> ownPtr)
	{
		std::lock_guard _lock(mFinishLineMutex);
		mFileCursorsQueue.push_back(fileCursor);
		mRawTransactionsBulk.push_back(line);
		if (mRawTransactionsBulk.size() >= REBUILD_BLOCK_INDEX_TASK_BULK_SIZE) {
			flush(ownPtr, false);
		}
	}

	void RebuildBlockIndexTask::flush(std::shared_ptr<RebuildBlockIndexTask> ownPtr, bool last/* = true, */)
	{
		std::lock_guard _lock(mFinishLineMutex);
		if (!mRawTransactionsBulk.size()) { return; }
		auto deserializeTask = make_shared<BatchDeserializeConfirmedTransactionTask>(std::move(mRawTransactionsBulk));
		// deserializeTask->setFinishCommand(new FinishedDeserializeForRebuildBlockIndexCommand(ownPtr));
		mBulkDeserializerTasks.push(deserializeTask);
		deserializeTask->scheduleTask(deserializeTask);
		
		if (!last) {
			mRawTransactionsBulk.reserve(REBUILD_BLOCK_INDEX_TASK_BULK_SIZE);
		}
	}

	void RebuildBlockIndexTask::finishBulk()
	{
		std::lock_guard lock(mFinishLineMutex);
		while (!mBulkDeserializerTasks.empty() && mBulkDeserializerTasks.front()->isTaskFinished())
		{
			auto& task = mBulkDeserializerTasks.front();
			const auto& confirmedTransactions = task->getConfirmedTransactions();
			const auto& rawTransactions = task->getRawTransactions();
			for (int i = 0; i < confirmedTransactions.size(); i++) {
				const auto& confirmedTransaction = confirmedTransactions[i];
				auto fileCursor = mFileCursorsQueue.front();

				std::shared_ptr<gradido::blockchain::NodeTransactionEntry> transactionEntry = std::make_shared<gradido::blockchain::NodeTransactionEntry>(
					confirmedTransaction,
					rawTransactions[i],
					mBlockchain,
					fileCursor
				);
				try {
					mBlockIndex->addIndicesForTransaction(transactionEntry, mPublicKeyIndex);
				}
				catch (std::exception& e) {
					LOG_F(FATAL, "%s, couldn't add indices for transaction: %s",
						e.what(),
						serialization::toJsonString(*transactionEntry->getConfirmedTransaction(), true).c_str()
					);
				}
				mFileCursorsQueue.pop_front();
			}			
			mBulkDeserializerTasks.pop();
		}		
	}

	bool RebuildBlockIndexTask::isPendingQueueEmpty()
	{
		finishBulk();
		return mBulkDeserializerTasks.empty();
	}
}