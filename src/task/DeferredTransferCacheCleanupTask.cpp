#include "DeferredTransferCacheCleanupTask.h"
#include "../blockchain/FileBasedProvider.h"

using namespace gradido;
using namespace blockchain;

namespace task {
	DeferredTransferCacheCleanupTask::DeferredTransferCacheCleanupTask(std::string_view communityName)
		: mCommunityName(communityName)
	{

	}
	DeferredTransferCacheCleanupTask::~DeferredTransferCacheCleanupTask()
	{

	}

	int DeferredTransferCacheCleanupTask::run()
	{
		auto community = FileBasedProvider::getInstance()->findBlockchain(mCommunityName);
		if (!community) {
			throw GradidoNullPointerException("cannot find community", "blockchain::Abstract", __FUNCTION__);
		}
		auto filedBasedBlockchain = std::dynamic_pointer_cast<FileBased>(community);
		if (!filedBasedBlockchain) {
			throw GradidoNullPointerException("blockchain isn't FileBased", "blockchain::FileBased", __FUNCTION__);
		}
		filedBasedBlockchain->cleanupDeferredTransferCache();
		return 0;
	}
}