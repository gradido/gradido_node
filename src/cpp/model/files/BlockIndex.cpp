#include "BlockIndex.h"
#include "../../SingletonManager/FileLockManager.h"
#include "../../SingletonManager/MemoryManager.h"

#include "Poco/File.h"
#include "Poco/FileStream.h"

namespace model {
	namespace files {

		

		void BlockIndex::DataBlock::writeIntoFile(Poco::FileOutputStream &file, crypto_generichash_state &state)
		{
			// write block type, transaction nr and address index count
			uint8_t size1 = sizeof(uint64_t) + sizeof(uint16_t);
			file.write((const char*)this, size1);
			
			// write address indices
			uint8_t size2 = sizeof(uint32_t) * addressIndicesCount;
			file.write((const char*)addressIndices, size2);

			// hashing 
			crypto_generichash_update(&state, (const unsigned char*)this, size1);
			crypto_generichash_update(&state, (const unsigned char*)addressIndices, size2);
		}

		void BlockIndex::DataBlock::readFromFile(Poco::FileInputStream &file, crypto_generichash_state &state)
		{

		}

		// **************************************************************************

		BlockIndex::BlockIndex(Poco::Path groupFolderPath, Poco::UInt32 blockNr)
			: mDataBlockSumSize(0)
		{
			char fileName[24]; memset(fileName, 0, 24);
#ifdef _MSC_VER
			sprintf_s(fileName, 24, "blk%08d.index", blockNr);
#else 
			sprintf(fileName, "blk%08d.index", blockNr);
#endif
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
			auto mm = MemoryManager::getInstance();

			auto fileBuffer = mm->getFreeMemory(mDataBlockSumSize + crypto_generichash_BYTES);

			if (!fl->tryLockTimeout(mFilename, 100)) {
				return false;
			}

			Poco::FileOutputStream file(mFilename);

			auto hash = mm->getFreeMemory(crypto_generichash_BYTES);

			crypto_generichash_state state;
			crypto_generichash_init(&state, nullptr, 0, crypto_generichash_BYTES);

			while (!mDataBlocks.empty())
			{
				auto block = mDataBlocks.front();
				mDataBlocks.pop();
				if (DATA_BLOCK == block->type) {
					uint32_t size = block->size();
					file.write((const char*)&size, sizeof(uint32_t));
				}
				else {
					file.write((const char*)block, block->size());
				}
				crypto_generichash_update(&state, (const unsigned char*)block, block->size());
				delete block;
			}

			crypto_generichash_final(&state, *hash, hash->size());
			file.write((const char*)*hash, hash->size());
			mm->releaseMemory(hash);

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