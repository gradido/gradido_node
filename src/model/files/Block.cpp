#include "Block.h"
#include "FileExceptions.h"

#include "../../SingletonManager/FileLockManager.h"
#include "../../SingletonManager/LoggerManager.h"
#include "../../SingletonManager/CacheManager.h"
#include "../../ServerGlobals.h"
#include "gradido_blockchain/lib/Profiler.h"

//#include "../../lib/BinTextConverter.h"
#include "gradido_blockchain/lib/DataTypeConverter.h"

#include "Poco/File.h"

#include <sodium.h>

namespace model {
	namespace files {
		Block::Block(Poco::Path groupFolderPath, Poco::UInt32 blockNr)
			: //mTimer(0, ServerGlobals::g_TimeoutCheck),
			  mBlockPath(groupFolderPath), mBlockNr(blockNr), mLastWrittenTransactionNr(0), mCurrentFileSize(0), mCurrentFileCursorReadPosition(0)
		{
			char fileName[16]; memset(fileName, 0, 16);
			sprintf(fileName, "blk%08d.dat", blockNr);
			mBlockPath.append(fileName);
			Poco::File file(mBlockPath);
			if (!file.exists()) {
				file.createFile();
			}

			//Poco::TimerCallback<Block> callback(*this, &Block::checkTimeout);
			//mTimer.start(callback);
			CacheManager::getInstance()->getFuzzyTimer()->addTimer("model::file::" + mBlockPath.toString(), this, ServerGlobals::g_TimeoutCheck, -1);
		}

		Block::~Block()
		{
			Poco::FastMutex::ScopedLock lock(mFastMutex);
			if (CacheManager::getInstance()->getFuzzyTimer()->removeTimer("model::file::" + mBlockPath.toString()) != 1) {
				printf("[model::files::~Block] error removing timer\n");
			}
			//printf("[model::files::~Block]\n");
			//mTimer.stop();
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
				mCurrentFileCursorReadPosition = telled;
			}
			mLastUsed = Poco::Timestamp();
			return mBlockFile;
		}
		/*
		void Block::checkTimeout(Poco::Timer& timer)
		{
			if (!mFastMutex.tryLock()) return;
			if (Poco::Timestamp() - mLastUsed > ServerGlobals::g_CacheTimeout) {
				mBlockFile = nullptr;
			}
			mFastMutex.unlock();
		}
		*/
		TimerReturn Block::callFromTimer()
		{
			if (!mFastMutex.tryLock()) return GO_ON;
			if (Poco::Timestamp() - mLastUsed > ServerGlobals::g_CacheTimeout) {
				mBlockFile = nullptr;
			}
			mFastMutex.unlock();
			return GO_ON;
		}
		
		std::unique_ptr<std::string> Block::readLine(Poco::UInt32 startReading)
		{
			// set also mCurrentFileSize
			auto fileStream = getOpenFile();
			auto minimalFileSize = sizeof(Poco::UInt16) + MAGIC_NUMBER_MINIMAL_TRANSACTION_SIZE;
			if (mCurrentFileSize <= minimalFileSize) {
				throw EndReachingException("file is smaller than minimal block size", mBlockPath.toString().data(), 0, minimalFileSize);
			}
			if (startReading > mCurrentFileSize - minimalFileSize) {
				throw EndReachingException("file is to small for read request", mBlockPath.toString().data(), startReading, minimalFileSize);
			}
			Profiler timeUsed;
			//Poco::FastMutex::ScopedLock lock(mFastMutex);
			auto fl = FileLockManager::getInstance();
			auto mm = MemoryManager::getInstance();
			std::string filePath = mBlockPath.toString();
			if (!fl->tryLockTimeout(filePath, 100)) {
				throw LockException("cannot lock file for reading", filePath.data());
			}
			
			Poco::UInt16 transactionSize = 0;
			// call seek only if it is really necessary 
			// for example if a block file is read in complete line by line on program startup
			// https://stackoverflow.com/questions/2438953/how-is-fseek-implemented-in-the-filesystem
			// "One observation I have made about fseek on Solaris, is that each call to it resets the read buffer of the FILE.The next read will then always read a full block(8K by default)."
			// https://bytes.com/topic/c/answers/218188-fseek-speed
			// "However, a side - effect of the fseek is the flushing of the buffer.Without
			//	the fseek(), your output will(actually, I suppose "can" is correct in the
			//		general sense) be buffered, and only written when the buffer fille.With
			//	the fseek(), you are forcing the buffer to be written for every character."
			// https://stackoverflow.com/questions/9349470/whats-the-difference-between-fseek-lseek-seekg-seekp
			// "The difference between the various seek functions is just the kind of file/stream objects on which they operate. 
			//  On Linux, seekg and fseek are probably implemented in terms of lseek."
			if (mCurrentFileCursorReadPosition != startReading) {
				fileStream->seekg(startReading);
				mCurrentFileCursorReadPosition = startReading;
			}
			
			fileStream->read((char*)&transactionSize, sizeof(Poco::UInt16));
			mCurrentFileCursorReadPosition += sizeof(Poco::UInt16);
			if (startReading + transactionSize > mCurrentFileSize) {
				fl->unlock(filePath);
				throw EndReachingException("file is to small for transaction size", mBlockPath.toString().data(), startReading, transactionSize);
			}
			std::unique_ptr<std::string> resultString(new std::string);
			resultString->reserve(transactionSize);
			// block wise read
			char buffer[4096];
			size_t readCursor = 0;
			while (fileStream->read(buffer, sizeof(buffer)) && transactionSize - readCursor >= 4096)
				resultString->append(buffer, sizeof(buffer));
			resultString->append(buffer, transactionSize - readCursor);

			//fileStream->read(resultString->data(), transactionSize);			
			mCurrentFileCursorReadPosition += transactionSize;
			fl->unlock(filePath);
			auto base64 = DataTypeConverter::binToBase64(resultString->data());
			return std::move(resultString);
		}


		Poco::Int32 Block::appendLine(const std::string* line)
		{
			std::vector<const std::string*> lines(1);
			lines[0] = line;

			return appendLines(lines)[0];

		}

		std::vector<Poco::UInt32> Block::appendLines(const std::vector<const std::string*>& lines)
		{
			//Poco::FastMutex::ScopedLock lock(mFastMutex);
			auto fl = FileLockManager::getInstance();
			auto mm = MemoryManager::getInstance();
			std::vector<Poco::UInt32> resultingCursorPositions;// (lines.size());
			resultingCursorPositions.reserve(lines.size());

			std::string filePath = mBlockPath.toString();
			if (!fl->tryLockTimeout(filePath, 100)) {
				throw LockException("couldn't lock file in time", filePath.data());
			}
			auto fileStream = getOpenFile();

			// read current hash
			auto hash = mm->getMemory(crypto_generichash_KEYBYTES);
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
				Poco::UInt16 size = (*itLines)->size();
				// check data type overflow
				assert((Poco::UInt32)size == (*itLines)->size());
				resultingCursorPositions.push_back(cursorPos);
				
				fileStream->write((char*)&size, sizeof(Poco::UInt16));
				fileStream->write((*itLines)->data(), size);
				mCurrentFileSize += sizeof(Poco::UInt16) + size;

				crypto_generichash_init(&state, nullptr, 0, crypto_generichash_BYTES);
				crypto_generichash_update(&state, *hash, hash->size());
				crypto_generichash_update(&state, (const unsigned char*)(*itLines)->data(), size);
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

			auto vfile = mm->getMemory(mCurrentFileSize);
			if (!vfile) {
				fl->unlock(filePath);
				lm->mErrorLogging.error("[%s] error reserve memory: %d", std::string(__FUNCTION__), mCurrentFileSize);
				return nullptr;
			}
			auto fileStream = getOpenFile();

			fileStream->seekg(0);
			fileStream->read(*vfile, mCurrentFileSize);

			fl->unlock(filePath);

			auto hash = mm->getMemory(crypto_generichash_KEYBYTES);
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

			auto hash = mm->getMemory(crypto_generichash_KEYBYTES);
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

		BlockAppendLineTask::BlockAppendLineTask(Poco::SharedPtr<Block> block, std::vector<const std::string*> lines)
			: task::CPUTask(ServerGlobals::g_WriteFileCPUScheduler), mTargetBlock(block), mLines(lines)
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
