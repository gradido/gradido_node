#include "BlockIndex.h"
#include "../../SingletonManager/FileLockManager.h"

#include "Poco/File.h"

namespace model {
	namespace files {
		BlockIndex::BlockIndex(Poco::Path groupFolderPath, Poco::UInt32 blockNr)
		{
			char fileName[16]; memset(fileName, 0, 16);
			sprintf(fileName, "blk%08d.index", blockNr);
			groupFolderPath.append(fileName);
			Poco::File file(groupFolderPath);
			if (!file.exists()) {
				file.createFile();
			}
			mFilename = groupFolderPath.toString();

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

			// TODO: calculate sha256 file hash

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