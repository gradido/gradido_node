#ifndef __GRADIDO_NODE_TASK_REBUILD_BLOCK_INDEX_TASK_H
#define __GRADIDO_NODE_TASK_REBUILD_BLOCK_INDEX_TASK_H

#include "CPUTask.h"
#include "../model/files/Block.h"
#include "gradido_blockchain/crypto/ByteArray.h"
#include "gradido_blockchain/data/compact/ConfirmedGradidoTx.h"
#include "gradido_blockchain/lib/DictionaryInterface.h"

#include <condition_variable>
#include <deque>
#include <memory>
#include <mutex>
#include <queue>

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
#define REBUILD_BLOCK_INDEX_TASK_BUFFER_SIZE 1024

namespace task {
	
	//! remove dependencie to CPUTask because this isn't really a cpu task, more are result storage,
	//! because it will start subsequent tasks of it own which will call back via command if finished
	class RebuildBlockIndexTask : public CPUTask, public model::files::IBlockBufferRead
	{
	public:
		RebuildBlockIndexTask(
			std::shared_ptr<cache::BlockIndex> blockIndex,
			uint32_t communityIdIndex
		);
		const char* getResourceType() const { return "RebuildBlockIndexTask"; };

		int run();
		
		virtual void finishedLine(uint16_t memStart, uint16_t size, int32_t fileCursor) override;
		virtual void flush() override;
		
		inline grdu_memory* getAlloc() { return &mReadInAllocator; }

	protected:
	
		uint8_t mBuffers[2][REBUILD_BLOCK_INDEX_TASK_BUFFER_SIZE];
		
		grdu_memory mReadInAllocator;
		std::mutex mWorkConfirmedMutex;
		gradido::data::compact::ConfirmedGradidoTx mConfirmedTx;
		std::condition_variable mConfirmedTxReadyCondition;
		std::shared_ptr<cache::BlockIndex> mBlockIndex;
		uint32_t mCommunityIdIndex;
		std::atomic<bool> mLastLineReaded;
	};

}

#endif // __GRADIDO_NODE_TASK_REBUILD_BLOCK_INDEX_TASK_H