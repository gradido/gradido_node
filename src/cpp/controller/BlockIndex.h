#ifndef __GRADIDO_NODE_CONTROLLER_BLOCK_INDEX_H
#define __GRADIDO_NODE_CONTROLLER_BLOCK_INDEX_H

#include "ControllerBase.h"
#include <vector>
#include <map>

namespace controller {

	/*!
	 * @author Dario Rekowski
	 * @date 2020-02-12
	 * @brief 
	 *
	 * map: uint32 file cursor, uint64 transaction nr 
	 * map: address index, file cursors
	 * map: year, map: month, address indices
	 */

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