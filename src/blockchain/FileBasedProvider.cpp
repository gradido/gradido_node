#include "FileBasedProvider.h"

namespace gradido {
	namespace blockchain {
		FileBasedProvider::FileBasedProvider()
		{

		}

		FileBasedProvider::~FileBasedProvider()
		{
			clear();
		}

		void FileBasedProvider::clear()
		{
			std::lock_guard _lock(mWorkMutex);
			mBlockchainsPerGroup.clear();
		}

		FileBasedProvider* FileBasedProvider::getInstance()
		{
			FileBasedProvider one;
			return &one;
		}

		std::shared_ptr<Abstract> FileBasedProvider::findBlockchain(std::string_view communityId)
		{
			std::lock_guard _lock(mWorkMutex);
			auto it = mBlockchainsPerGroup.find(communityId);
			if (it != mBlockchainsPerGroup.end()) {
				return it->second;
			}
			// load new blockchain if it exist in config
		}
	}
}