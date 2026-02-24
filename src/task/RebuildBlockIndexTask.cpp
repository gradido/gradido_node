#include "RebuildBlockIndexTask.h"
#include "BatchDeserializeConfirmedTransactionTask.h"
#include "../ServerGlobals.h"
#include "../blockchain/FileBased.h"

#include "gradido_blockchain/AppContext.h"
#include "gradido_blockchain/lib/Profiler.h"
#include "gradido_blockchain/memory/Block.h"
#include "gradido_blockchain/serialization/toJsonString.h"
#include "gradido_protobuf_zig.h"

#include "loguru/loguru.hpp"
#include "magic_enum/magic_enum.hpp"

#include <mutex>

using namespace magic_enum;
using memory::Block;
using gradido::blockchain::FileBased;
using gradido::data::compact::ConfirmedGradidoTx;
using std::shared_ptr, std::make_shared, std::unique_lock;
using ServerGlobals::g_CPUScheduler;

namespace task {
	RebuildBlockIndexTask::RebuildBlockIndexTask(
		std::shared_ptr<cache::BlockIndex> blockIndex,
		uint32_t communityIdIndex
	): task::CPUTask(g_CPUScheduler), mBlockIndex(blockIndex), mCommunityIdIndex(communityIdIndex), 
		mLastLineReaded(false)
	{
		grdu_memory_init_static(&mReadInAllocator, mBuffers[0], REBUILD_BLOCK_INDEX_TASK_BUFFER_SIZE);
	}

	int RebuildBlockIndexTask::run()
	{
		unique_lock lock(mWorkConfirmedMutex);
		mConfirmedTxReadyCondition.wait(lock, [&] { return mLastLineReaded.load();  });
		return 0;
	}

	void RebuildBlockIndexTask::finishedLine(uint16_t memStart, uint16_t size, int32_t fileCursor)
	{
		grdu_memory decodeMemoryAlloc;
		grdu_memory_init_static(&decodeMemoryAlloc, mBuffers[1], REBUILD_BLOCK_INDEX_TASK_BUFFER_SIZE);
		grdw_confirmed_transaction tx{};
		auto encodeResult = grdw_confirmed_transaction_decode(&decodeMemoryAlloc, &tx, mBuffers[0], size);
		if (GRDW_ENCODING_ERROR_SUCCESS != encodeResult.state) {
			LOG_F(ERROR, "decode error: %s", enum_name(encodeResult.state).data());
			throw GradidoNodeInvalidDataException("error deserialize confirmed transaction");
		}
		
		grdw_transaction_body body{};
		grdu_memory_init_static(&decodeMemoryAlloc, mBuffers[0], REBUILD_BLOCK_INDEX_TASK_BUFFER_SIZE);
		encodeResult = grdw_transaction_body_decode(&decodeMemoryAlloc, &body, tx.transaction.body_bytes, tx.transaction.body_bytes_size);
		if (GRDW_ENCODING_ERROR_SUCCESS != encodeResult.state) {
			LOG_F(ERROR, "body decode error: %s", enum_name(encodeResult.state).data());
			throw GradidoNodeInvalidDataException("error deserialize transaction body");
		}

		auto compactConfirmedTx = ConfirmedGradidoTx::fromGrdw(&tx, &body, mCommunityIdIndex, *gradido::g_appContext);
		mBlockIndex->addIndicesForTransaction(compactConfirmedTx);
		mBlockIndex->addFileCursorForTransaction(compactConfirmedTx.txNr, fileCursor);

		grdu_memory_init_static(&mReadInAllocator, mBuffers[0], REBUILD_BLOCK_INDEX_TASK_BUFFER_SIZE);
	}

	void RebuildBlockIndexTask::flush() 
	{
		mLastLineReaded = true;
		mConfirmedTxReadyCondition.notify_one();
	}		
}