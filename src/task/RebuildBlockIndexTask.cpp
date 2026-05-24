#include "RebuildBlockIndexTask.h"
#include "BatchDeserializeConfirmedTransactionTask.h"
#include "../ServerGlobals.h"
#include "../blockchain/FileBased.h"

#include "gradido_blockchain_core/memory.h"
#include "gradido_blockchain_core/data/wire/confirmed_transaction.h"
#include "gradido_blockchain_core/data/wire/transaction_body.h"
#include "gradido_blockchain/AppContext.h"
#include "gradido_blockchain/memory/Block.h"
#include "gradido_blockchain/serialization/toJsonString.h"

#include "loguru/loguru.hpp"
#include "magic_enum/magic_enum.hpp"

#include <mutex>

using gradido::g_appContext;
using gradido::blockchain::FileBased;
using gradido::data::compact::ConfirmedGradidoTx;
using namespace magic_enum;
using memory::Block;
using std::shared_ptr, std::make_shared, std::unique_lock;
using ServerGlobals::g_CPUScheduler;

namespace task {
	RebuildBlockIndexTask::RebuildBlockIndexTask(
		std::shared_ptr<cache::BlockIndex> blockIndex,
		uint32_t communityIdIndex
	): task::CPUTask(g_CPUScheduler), mBlockIndex(blockIndex), mCommunityIdIndex(communityIdIndex), 
		mLastLineReaded(false)
	{
		grd_memory_init_arena_static(&mReadInAllocator, mBuffers[0], REBUILD_BLOCK_INDEX_TASK_BUFFER_SIZE);
	}

	int RebuildBlockIndexTask::run()
	{
		unique_lock lock(mWorkConfirmedMutex);
		mConfirmedTxReadyCondition.wait(lock, [&] { return mLastLineReaded.load();  });
		return 0;
	}

	void RebuildBlockIndexTask::finishedLine(uint16_t memStart, uint16_t size, int32_t fileCursor)
	{
		grd_memory decodeMemoryAlloc;
		grd_memory_init_arena_static(&decodeMemoryAlloc, mBuffers[1], REBUILD_BLOCK_INDEX_TASK_BUFFER_SIZE);
		grdw_confirmed_transaction tx{};
		grd_memory_block src = { .data = mBuffers[0], .size = size };
		auto result = grdw_confirmed_transaction_decode(&tx, &src, &decodeMemoryAlloc);
		if (GRD_SUCCESS != result) {
			LOG_F(ERROR, "decode error: %s", enum_name(result).data());
			throw GradidoNodeInvalidDataException("error deserialize confirmed transaction");
		}
		
		grdw_transaction_body body{};
		grd_memory_init_arena_static(&decodeMemoryAlloc, mBuffers[0], REBUILD_BLOCK_INDEX_TASK_BUFFER_SIZE);
		result = grdw_transaction_body_decode(&body, &tx.transaction.body_bytes, &decodeMemoryAlloc);
		if (GRD_SUCCESS != result) {
			LOG_F(ERROR, "body decode error: %s", enum_name(result).data());
			throw GradidoNodeInvalidDataException("error deserialize transaction body");
		}

		auto compactConfirmedTx = ConfirmedGradidoTx::fromGrdw(&tx, &body, mCommunityIdIndex, *g_appContext);
		mBlockIndex->addIndicesForTransaction(compactConfirmedTx, g_appContext->getCommunityContext(mCommunityIdIndex).getBlockchain()->getPublicKeyDictionary());
		mBlockIndex->addFileCursorForTransaction(compactConfirmedTx.txNr, fileCursor);

		grd_memory_init_arena_static(&mReadInAllocator, mBuffers[0], REBUILD_BLOCK_INDEX_TASK_BUFFER_SIZE);
	}

	void RebuildBlockIndexTask::flush() 
	{
		mLastLineReaded = true;
		mConfirmedTxReadyCondition.notify_one();
	}		
}