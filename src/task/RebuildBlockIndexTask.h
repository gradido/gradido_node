#ifndef __GRADIDO_NODE_TASK_REBUILD_BLOCK_INDEX_TASK_H
#define __GRADIDO_NODE_TASK_REBUILD_BLOCK_INDEX_TASK_H

#include "CPUTask.h"
#include "gradido_blockchain/lib/DictionaryInterface.h"

#include <memory>
#include <queue>
#include <deque>
#include <mutex>

namespace gradido {
	namespace blockchain {
		class FileBased;
		class NodeTransactionEntry;
	}
}

namespace memory {
	class Block;
	using ConstBlockPtr = std::shared_ptr<const Block>;
}

namespace cache {
	class BlockIndex;
}

// TODO: maybe put into config
#define REBUILD_BLOCK_INDEX_TASK_BULK_SIZE 1000

namespace task {
	class BatchDeserializeConfirmedTransactionTask;


	//! remove dependencie to CPUTask because this isn't really a cpu task, more are result storage,
	//! because it will start subsequent tasks of it own which will call back via command if finished
	class RebuildBlockIndexTask : public CPUTask
	{
	public:
		RebuildBlockIndexTask(
			std::shared_ptr<const gradido::blockchain::FileBased> blockchain, 
			std::shared_ptr<cache::BlockIndex> blockIndex,
			IMutableDictionary<memory::ConstBlockPtr>& publicKeyDictionary
		);
		const char* getResourceType() const { return "RebuildBlockIndexTask"; };

		int run();
		//! \param line will be moved
		void pushLine(int32_t fileCursor, memory::ConstBlockPtr line, std::shared_ptr<RebuildBlockIndexTask> ownPtr);
		// flush batch buffer
		void flush(std::shared_ptr<RebuildBlockIndexTask> ownPtr, bool last = true);
		// called from DeserializeConfirmedTransactionTask, will process queue from begin as long current entry was already deserialized
		void finishBulk();

		bool isPendingQueueEmpty();

	protected:
		std::shared_ptr<const gradido::blockchain::FileBased> mBlockchain;
		std::shared_ptr<cache::BlockIndex> mBlockIndex;
		IMutableDictionary<memory::ConstBlockPtr>& mPublicKeyIndex;
		std::queue<std::shared_ptr<BatchDeserializeConfirmedTransactionTask>> mBulkDeserializerTasks;
		std::deque<int32_t> mFileCursorsQueue;
		std::vector<memory::ConstBlockPtr> mRawTransactionsBulk;
		std::recursive_mutex mFinishLineMutex;
		std::atomic<size_t> mActiveDeserializerTasks;
	};


	class FinishedDeserializeForRebuildBlockIndexCommand : public Command
	{
	public:
		FinishedDeserializeForRebuildBlockIndexCommand(std::shared_ptr<RebuildBlockIndexTask> parent) : mParent(parent) {}
		int taskFinished(Task* task) override
		{
			mParent->finishBulk();
			return 0;
		}

	protected:
		std::shared_ptr<RebuildBlockIndexTask> mParent;
	};
}

#endif // __GRADIDO_NODE_TASK_REBUILD_BLOCK_INDEX_TASK_H