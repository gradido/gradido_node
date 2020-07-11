#include "Block.h"

#include "../../SingletonManager/FileLockManager.h"
#include "../../SingletonManager/LoggerManager.h"
#include "../../ServerGlobals.h"
#include "../../lib/Profiler.h"

#include "../../lib/BinTextConverter.h"

#include "Poco/File.h"

#include <sodium.h>

namespace model {
	namespace files {
		Block::Block(Poco::Path groupFolderPath, Poco::UInt32 blockNr)
			: mBlockPath(groupFolderPath), mBlockNr(blockNr), mLastWrittenTransactionNr(0), mCurrentFileSize(0)
		{
			char fileName[16]; memset(fileName, 0, 16);
			sprintf(fileName, "blk%08d.dat", blockNr);
			mBlockPath.append(fileName);
			Poco::File file(mBlockPath);
			if (!file.exists()) {
				file.createFile();
			}

			TimeoutManager::getInstance()->registerTimeout(this);
		}

		Block::~Block()
		{
			TimeoutManager::getInstance()->unregisterTimeout(this);
		}

		Poco::SharedPtr<Poco::FileStream> Block::getOpenFile()
		{
			Poco::FastMutex::ScopedLock lock(mFastMutex);
			if (mBlockFile.isNull()) {
				mBlockFile = new Poco::FileStream(mBlockPath.toString());
				mBlockFile->seekg(0, std::ios_base::end);
				auto telled = mBlockFile->tellg();
				if (telled && telled > crypto_generichash_KEYBYTES) {
					mCurrentFileSize = (Poco::UInt32)mBlockFile->tellg() - crypto_generichash_KEYBYTES;
				}
			}
			mLastUsed = Poco::Timestamp();
			return mBlockFile;
		}

		void Block::checkTimeout()
		{
			Poco::FastMutex::ScopedLock lock(mFastMutex);
			if (Poco::Timestamp() - mLastUsed > ServerGlobals::g_CacheTimeout) {
				mBlockFile = nullptr;
			}
		}

		int Block::readLine(Poco::UInt32 startReading, std::string& resultString)
		{
			if (startReading > mCurrentFileSize - (sizeof(Poco::UInt16) + 25)) {
				return -3;
			}
			Profiler timeUsed;
			//Poco::FastMutex::ScopedLock lock(mFastMutex);
			auto fl = FileLockManager::getInstance();
			auto mm = MemoryManager::getInstance();
			std::string filePath = mBlockPath.toString();
			if (!fl->tryLockTimeout(filePath, 100)) {
				return -1;
			}
			auto fileStream = getOpenFile();
			Poco::UInt16 transactionSize = 0;
			fileStream->seekg(startReading);
			fileStream->read((char*)&transactionSize, sizeof(Poco::UInt16));
			if (startReading + transactionSize > mCurrentFileSize) {
				fl->unlock(filePath);
				return -2;
			}
			std::vector<char> lineVector(transactionSize);
			fileStream->read(lineVector.data(), transactionSize);

			fl->unlock(filePath);

			resultString = std::string(lineVector.begin(), lineVector.end());
			return 0;
		}


		Poco::Int32 Block::appendLine(const std::string& line)
		{
			std::vector<std::string> lines(1);
			lines[0] = line;

			return appendLines(lines)[0];

		}

		std::vector<Poco::UInt32> Block::appendLines(const std::vector<std::string>& lines)
		{
			//Poco::FastMutex::ScopedLock lock(mFastMutex);
			auto fl = FileLockManager::getInstance();
			auto mm = MemoryManager::getInstance();
			std::vector<Poco::UInt32> resultingCursorPositions;// (lines.size());
			resultingCursorPositions.reserve(lines.size());

			std::string filePath = mBlockPath.toString();
			if (!fl->tryLockTimeout(filePath, 100)) {
				resultingCursorPositions.push_back(-1);
				return resultingCursorPositions;
			}
			auto fileStream = getOpenFile();

			// read current hash
			auto hash = mm->getFreeMemory(crypto_generichash_KEYBYTES);
			memset(*hash, 0, crypto_generichash_KEYBYTES);
			if (mCurrentFileSize > 0) {
				fileStream->seekg(mCurrentFileSize);
				fileStream->read(*hash, crypto_generichash_KEYBYTES);
			}
			fileStream->seekp(mCurrentFileSize);

			// create new hash
			crypto_generichash_state state;
			

			for (auto itLines = lines.begin(); itLines != lines.end(); itLines++)
			{
				//fileStream->seekp(mCurrentFileSize);
				Poco::Int32 cursorPos = fileStream->tellp();
				Poco::UInt16 size = itLines->size();
				// check data type overflow
				if ((Poco::UInt32)size != itLines->size()) {
					fl->unlock(filePath);
					resultingCursorPositions.push_back(-2);
					mm->releaseMemory(hash);
					return resultingCursorPositions;
				}
				resultingCursorPositions.push_back(cursorPos);

				fileStream->write((char*)&size, sizeof(Poco::UInt16));
				fileStream->write(itLines->data(), itLines->size());
				mCurrentFileSize += sizeof(Poco::UInt16) + itLines->size();

				crypto_generichash_init(&state, nullptr, 0, crypto_generichash_BYTES);
				crypto_generichash_update(&state, *hash, hash->size());
				crypto_generichash_update(&state, (const unsigned char*)itLines->data(), itLines->size());				
				crypto_generichash_final(&state, *hash, hash->size());
			}

			// write at end of file
			fileStream->write(*hash, hash->size());

			fl->unlock(filePath);
			mm->releaseMemory(hash);
			return resultingCursorPositions;
		}

		MemoryBin* Block::calculateHash()
		{
			Profiler timeUsed;
			auto mm = MemoryManager::getInstance();
			auto fl = FileLockManager::getInstance();
			auto lm = LoggerManager::getInstance();

			if (mCurrentFileSize == 0) {
				return nullptr;
			}

			std::string filePath = mBlockPath.toString();
			if (!fl->tryLockTimeout(filePath, 100)) {
				lm->mErrorLogging.error("[%s] error locking file: %s", std::string(__FUNCTION__), filePath);
				return nullptr;
			}

			auto vfile = mm->getFreeMemory(mCurrentFileSize);
			if (!vfile) {
				fl->unlock(filePath);
				lm->mErrorLogging.error("[%s] error reserve memory: %d", std::string(__FUNCTION__), mCurrentFileSize);
				return nullptr;
			}
			auto fileStream = getOpenFile();

			fileStream->seekg(0);
			fileStream->read(*vfile, mCurrentFileSize);

			fl->unlock(filePath);

			auto hash = mm->getFreeMemory(crypto_generichash_KEYBYTES);
			memset(*hash, 0, hash->size());
			size_t cursor = 0;
			const unsigned char* vfilep = *vfile;
			crypto_generichash_state state;
			

			while (cursor < mCurrentFileSize) {
				Poco::UInt16 size;
				memcpy(&size, &vfilep[cursor], sizeof(Poco::UInt16));
				cursor += sizeof(Poco::UInt16);
				crypto_generichash_init(&state, nullptr, 0, crypto_generichash_BYTES);
				crypto_generichash_update(&state, *hash, hash->size());
				crypto_generichash_update(&state, &vfilep[cursor], size);
				crypto_generichash_final(&state, *hash, hash->size());
				cursor += size;
			}
			mm->releaseMemory(vfile);
			
			lm->mSpeedLogging.information("%s used for calculate hash for block: %s", timeUsed.string(), filePath);
			return hash;
		}

		bool Block::validateHash()
		{
			auto mm = MemoryManager::getInstance();
			auto fl = FileLockManager::getInstance();
			auto lm = LoggerManager::getInstance();

			std::string filePath = mBlockPath.toString();
			if (!fl->tryLockTimeout(filePath, 100)) {
				lm->mErrorLogging.error("[%s] error locking file: %s", std::string(__FUNCTION__), filePath);
				return false;
			}
			auto fileStream = getOpenFile();
			fileStream->seekg(mCurrentFileSize);

			auto hash = mm->getFreeMemory(crypto_generichash_KEYBYTES);
			fileStream->read(*hash, crypto_generichash_KEYBYTES);

			fl->unlock(filePath);
	
			auto hash2 = calculateHash();
			bool result = false;
			if (0 == sodium_memcmp(*hash, *hash2, crypto_generichash_KEYBYTES)) {
				result = true;
			}

			mm->releaseMemory(hash);
			mm->releaseMemory(hash2);
			return result;
		}


		// *********************** TASKS ***********************************

		BlockAppendLineTask::BlockAppendLineTask(Poco::SharedPtr<Block> block, std::vector<std::string> lines)
			: UniLib::controller::CPUTask(ServerGlobals::g_WriteFileCPUScheduler), mTargetBlock(block), mLines(lines)
		{

		}

		int BlockAppendLineTask::run()
		{
#ifdef _UNI_LIB_DEBUG
			setName(mTargetBlock->getBlockPath().data());
#endif 
			auto lm = LoggerManager::getInstance();
			mCursorPositions = mTargetBlock->appendLines(mLines);

			return 0;
		}
	}
}
