#include "BlockIndex.h"
#include "../../SingletonManager/FileLockManager.h"

namespace model {
	namespace files {
		BlockIndex::BlockIndex(const std::string& filename)
			: mFilename(filename)
		{

		}

		BlockIndex::~BlockIndex()
		{
			while (!mDataBlocks.empty()) 
			{
				auto block = mDataBlocks.front();
				mDataBlocks.pop();
				delete block;
			}
		}

		bool BlockIndex::writeToFile()
		{
			auto fl = FileLockManager::getInstance();
			if (!fl->tryLockTimeout(mFilename, 100)) {
				return false;
			}

			Poco::FileOutputStream file(mFilename);

			while (!mDataBlocks.empty())
			{
				auto block = mDataBlocks.front();
				mDataBlocks.pop();
				file.write((const char*)block, block->size());
				delete block;
			}
			fl->unlock(mFilename);
			return true;
		}

		bool BlockIndex::readFromFile(controller::BlockIndex* receiver)
		{
			auto fl = FileLockManager::getInstance();
			if (!fl->tryLockTimeout(mFilename, 100)) {
				return false;
			}

			Poco::FileInputStream file(mFilename);

			// read type (uint8_t)
			// read block
			// call receiver
			
			fl->unlock(mFilename);
			return true;
		}
	}

}