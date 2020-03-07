#ifndef __GRADIDO_NODE_CONTROLLER_BLOCK_INDEX_H
#define __GRADIDO_NODE_CONTROLLER_BLOCK_INDEX_H

#include "ControllerBase.h"
#include <vector>
#include <map>

namespace controller {
	class BlockIndex : public ControllerBase
	{
	public:
		BlockIndex();
		~BlockIndex();

		bool addFileCursorStart(int64_t ktoIndex, int32_t fileCursorStart);

	protected:
		struct BlockIndexEntry 
		{
			std::vector<int32_t> fileCursorStarts;
			int64_t ktoIndex;
		};

		int64_t mKtoIndexLowest;
		int64_t mKtoIndexHighest;

		std::map<int64_t, BlockIndexEntry*> mBlockIndices;
	};
}

#endif //__GRADIDO_NODE_CONTROLLER_BLOCK_INDEX_H