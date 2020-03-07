#include "BlockIndex.h"

#include "Poco/Logger.h"
#include "../SingletonManager/LoggerManager.h"

namespace controller {


	BlockIndex::BlockIndex()
		: mKtoIndexLowest(-1), mKtoIndexHighest(0)
	{

	}

	BlockIndex::~BlockIndex()
	{
		for (auto it = mBlockIndices.begin(); it != mBlockIndices.end(); it++) {
			delete it->second;
		}
		mBlockIndices.clear();
	}

	bool BlockIndex::addFileCursorStart(int64_t ktoIndex, int32_t fileCursorStart)
	{
		auto lm = LoggerManager::getInstance();
		Poco::FastMutex::ScopedLock lock(mWorkingMutex);

		auto it = mBlockIndices.find(ktoIndex);
		BlockIndexEntry* entry = nullptr;
		if (it == mBlockIndices.end()) {
			entry = new BlockIndexEntry;
			auto pair = mBlockIndices.insert(std::pair<int64_t, BlockIndexEntry*>(ktoIndex, entry));
			if (pair.second == false) {
				return false;
			}
			it = pair.first;
			entry->ktoIndex = ktoIndex;
			if (ktoIndex > mKtoIndexHighest) {
				mKtoIndexHighest = ktoIndex;
			}
			if (-1 == mKtoIndexLowest || ktoIndex < mKtoIndexLowest) {
				mKtoIndexLowest = ktoIndex;
			}
		}
		if (it != mBlockIndices.end()) {
			if (!entry->fileCursorStarts.size() || entry->fileCursorStarts.back() < fileCursorStart) {
				entry->fileCursorStarts.push_back(fileCursorStart);
				return true;
			}
			else {
				lm->mErrorLogging.warning("[BlockIndex::addFileCursorStart] out of order file cursor");
				return false;
			}
		}
		lm->mErrorLogging.warning("[BlockIndex::addFileCursorStart] error by creating new map entry");
	
		return false;
	}

}