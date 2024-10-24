#ifndef __GRADIDO_NODE_TASK_DEFERRED_TRANSFER_CACHE_CLEANUP_TASK_H
#define __GRADIDO_NODE_TASK_DEFERRED_TRANSFER_CACHE_CLEANUP_TASK_H

#include "CPUTask.h"

namespace task {
	class DeferredTransferCacheCleanupTask : public CPUTask
	{
	public:
		DeferredTransferCacheCleanupTask(std::string_view communityName);
		~DeferredTransferCacheCleanupTask();

		//! \brief called from task scheduler, maybe from another thread
		//! \return if return 0, mark task as finished
		virtual int run();

		virtual const char* getResourceType() const { return "DeferredTransferCacheCleanupTask"; }
	protected:
		std::string mCommunityName;
	};
}

#endif //__GRADIDO_NODE_TASK_DEFERRED_TRANSFER_CACHE_CLEANUP_TASK_H