#include "Block.h"
#include "FileExceptions.h"

#include "../../blockchain/FileBased.h"
#include "../../blockchain/NodeTransactionEntry.h"
#include "../../ServerGlobals.h"
#include "../../SingletonManager/FileLockManager.h"
#include "../../SingletonManager/CacheManager.h"

#include "gradido_blockchain/data/ConfirmedTransaction.h"
#include "gradido_blockchain/interaction/deserialize/Context.h"
#include "gradido_blockchain/lib/Profiler.h"
#include "gradido_blockchain/lib/DataTypeConverter.h"

#include "loguru/loguru.hpp"

#include <sodium.h>
#include <sstream>
#include <filesystem>
#include <regex>

using namespace gradido::interaction;

namespace model {
	namespace files {
		Block::Block(std::string_view groupFolderPath, uint32_t blockNr)
			: //mTimer(0, ServerGlobals::g_TimeoutCheck),
			  mBlockPath(groupFolderPath), mBlockNr(blockNr), mLastWrittenTransactionNr(0), mCurrentFileSize(0)
		{
			std::stringstream fileNameStream;
			fileNameStream << "/blk" << std::setw(8) << std::setfill('0') << blockNr << ".dat";
			mBlockPath.append(fileNameStream.str());

			getOpenFile();		

			CacheManager::getInstance()->getFuzzyTimer()->addTimer("model::file::" + mBlockPath, this, ServerGlobals::g_TimeoutCheck, -1);
		}

		Block::~Block()
		{
			std::scoped_lock lock(mFastMutex);
			auto result = CacheManager::getInstance()->getFuzzyTimer()->removeTimer("model::file::" + mBlockPath);
			if (result != 1 && result != -1) {
				LOG_F(ERROR, "error removing timer");
			}
		}

		std::shared_ptr<std::fstream> Block::getOpenFile()
		{
			//printf("Block::getOpenFile: %s\n", mBlockPath.toString().data());
			std::scoped_lock _lock(mFastMutex);
			if (!mBlockFile) {
				// create file if not exist
				if (!std::filesystem::exists(mBlockPath)) {
					std::ofstream ofs(mBlockPath);
					ofs.close();
				}
				// ate: at end	The output position starts at the end of the file.
				// out: for creating file if not exist
				mBlockFile = std::make_shared<std::fstream>(mBlockPath, std::fstream::binary | std::fstream::ate | std::fstream::out | std::fstream::in);
				if (!mBlockFile->is_open()) {
					throw OpenFileException("cannot open block file", mBlockPath.data(), mBlockFile->rdstate());
				}
				auto telled = mBlockFile->tellg();
				if (telled && telled > crypto_generichash_KEYBYTES) {
					mCurrentFileSize = (uint32_t)mBlockFile->tellg() - crypto_generichash_KEYBYTES;
				}
			}
			mLastUsed = std::chrono::system_clock::now();
			return mBlockFile;
		}

		TimerReturn Block::callFromTimer()
		{
			if (!mFastMutex.try_lock()) return TimerReturn::GO_ON;
			if (std::chrono::system_clock::now() - mLastUsed > ServerGlobals::g_CacheTimeout) {
				mBlockFile = nullptr;
			}
			mFastMutex.unlock();
			return TimerReturn::GO_ON;
		}

		uint16_t Block::readLine(uint32_t startReading, memory::BlockPtr* buffer)
		{
			auto fileStream = getOpenFile();
			if (fileStream->fail()) {
				throw std::runtime_error("[model::files::Block::readLine] file stream is failing!");
			}
			auto minimalFileSize = sizeof(uint16_t) + MAGIC_NUMBER_MINIMAL_TRANSACTION_SIZE;
			if (mCurrentFileSize <= minimalFileSize) {
				throw EndReachingException("file is smaller than minimal block size", mBlockPath.data(), 0, minimalFileSize);
			}
			if (startReading > mCurrentFileSize - minimalFileSize) {
				throw EndReachingException("file is to small for read request", mBlockPath.data(), startReading, minimalFileSize);
			}
			Profiler timeUsed;
			//Poco::FastMutex::ScopedLock lock(mFastMutex);
			auto fl = FileLockManager::getInstance();			
			if (!fl->tryLockTimeout(mBlockPath, 100)) {
				throw LockException("cannot lock file for reading", mBlockPath.data());
			}

			uint16_t transactionSize = 0;
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
			if (fileStream->tellg() != startReading) {
				fileStream->seekg(startReading, std::ios_base::beg);
			}
			//printf("state before read transaction size: %d\n", fileStream->rdstate());
			fileStream->read((char*)&transactionSize, sizeof(uint16_t));
			if (startReading + transactionSize > mCurrentFileSize) {
				fl->unlock(mBlockPath);
				throw EndReachingException("file is to small for transaction size", mBlockPath.data(), startReading, transactionSize);
			}
			if (transactionSize < minimalFileSize) {
				fl->unlock(mBlockPath);
				throw InvalidReadBlockSize("transactionSize is to small to contain a transaction", mBlockPath.data(), startReading, transactionSize);
			}
			*buffer = std::make_shared<memory::Block>(transactionSize);
			fileStream->read((char*)(*buffer)->data(), transactionSize);
			fl->unlock(mBlockPath);
			return transactionSize;
		}
		
		std::shared_ptr<memory::Block> Block::readLine(uint32_t startReading)
		{
			// set also mCurrentFileSize
			//printf("[Block::readLine] cursor: %d\n", startReading);
			std::shared_ptr<memory::Block> result;
			auto transactionSize = readLine(startReading, &result);
			return result;
		}


		int32_t Block::appendLine(memory::ConstBlockPtr line)
		{
			std::vector<memory::ConstBlockPtr> lines(1);
			lines[0] = line;

			return appendLines(lines)[0];

		}

		std::vector<uint32_t> Block::appendLines(const std::vector<memory::ConstBlockPtr>& lines)
		{
			//Poco::FastMutex::ScopedLock lock(mFastMutex);
			auto fl = FileLockManager::getInstance();			
			std::vector<uint32_t> resultingCursorPositions;// (lines.size());
			resultingCursorPositions.reserve(lines.size());

			if (!fl->tryLockTimeout(mBlockPath, 100)) {
				throw LockException("couldn't lock file in time", mBlockPath.data());
			}
			
			auto fileStream = getOpenFile();

			// read current hash
			unsigned char hash[crypto_generichash_KEYBYTES];

			memset(hash, 0, crypto_generichash_KEYBYTES);
			if (mCurrentFileSize > 0) {
				fileStream->seekg(mCurrentFileSize);
				fileStream->read((char*)hash, crypto_generichash_KEYBYTES);
			}
			fileStream->seekp(mCurrentFileSize);

			for (auto itLines = lines.begin(); itLines != lines.end(); itLines++)
			{
				//fileStream->seekp(mCurrentFileSize);
				int32_t cursorPos = fileStream->tellp();
				uint16_t size = (*itLines)->size();
				// check data type overflow
				assert((uint32_t)size == (*itLines)->size());

				resultingCursorPositions.push_back(cursorPos);
				
				fileStream->write((char*)&size, sizeof(uint16_t));
				fileStream->write((const char*)(*itLines)->data(), size);
				mCurrentFileSize += sizeof(uint16_t) + size;

				calculateOneHashStep(hash, (const unsigned char*)(*itLines)->data(), size);
				LOG_F(1, "block part hash in %s: %s", mBlockPath.data(), memory::Block(crypto_generichash_KEYBYTES, hash).convertToHex().data());
			}

			// write at end of file
			fileStream->write((const char*)hash, sizeof hash);
			fileStream->flush();
			fl->unlock(mBlockPath);
			return resultingCursorPositions;
		}

		std::shared_ptr<memory::Block> Block::calculateHash()
		{
			Profiler timeUsed;
			auto fl = FileLockManager::getInstance();
			
			if (mCurrentFileSize == 0) {
				return nullptr;
			}

			if (!fl->tryLockTimeout(mBlockPath, 100)) {
//				lm->mErrorLogging.error("[%s] error locking file: %s", std::string(__FUNCTION__), filePath);
				return nullptr;
			}
			// TODO: not good reading whole file into memory which can be up to 128 MByte
			// use buffer approach instead and expect underlying os caching file reading efficiently
			memory::Block vfile(mCurrentFileSize);
			if (!vfile) {
				fl->unlock(mBlockPath);
				LOG_F(ERROR, "error reserve memory: %d", mCurrentFileSize);
				return nullptr;
			}
			auto fileStream = getOpenFile();

			fileStream->seekg(0);
			fileStream->read((char*)vfile.data(), mCurrentFileSize);

			fl->unlock(mBlockPath);

			auto hash = std::make_shared<memory::Block>(crypto_generichash_KEYBYTES);
			size_t cursor = 0;
			const unsigned char* vfilep = vfile.data();

			while (cursor < mCurrentFileSize) {
				uint16_t size;
				memcpy(&size, &vfilep[cursor], sizeof(uint16_t));
				cursor += sizeof(uint16_t);
				calculateOneHashStep(*hash, &vfilep[cursor], size);
				cursor += size;
			}
			
			// lm->mSpeedLogging.information("%s used for calculate hash for block: %s", timeUsed.string(), filePath);
			return hash;
		}

		bool Block::validateHash()
		{
			auto fl = FileLockManager::getInstance();

			if (!fl->tryLockTimeout(mBlockPath, 100)) {
				LOG_F(ERROR, "error locking file: %s", mBlockPath);
				return false;
			}
			auto fileStream = getOpenFile();
			fileStream->seekg(mCurrentFileSize);

			unsigned char hash[crypto_generichash_KEYBYTES];
			fileStream->read((char*)hash, crypto_generichash_KEYBYTES);

			fl->unlock(mBlockPath);
	
			auto hash2 = calculateHash();
			bool result = false;
			if (0 == sodium_memcmp(hash, *hash2, crypto_generichash_KEYBYTES)) {
				result = true;
			}

			return result;
		}

		std::shared_ptr<RebuildBlockIndexTask> Block::rebuildBlockIndex(std::shared_ptr<const gradido::blockchain::FileBased> blockchain)
		{			
			auto fl = FileLockManager::getInstance();
			std::shared_ptr<RebuildBlockIndexTask> rebuildTask = std::make_shared<RebuildBlockIndexTask>(blockchain);
			
			int32_t fileCursor = 0;
			std::shared_ptr<memory::Block> readBuffer;
			unsigned char hash[crypto_generichash_KEYBYTES];
			memset(hash, 0, sizeof hash);

			// read in every line
			while (fileCursor + sizeof(uint16_t) + MAGIC_NUMBER_MINIMAL_TRANSACTION_SIZE <= mCurrentFileSize) {
				auto lineSize = readLine(fileCursor, &readBuffer);
				rebuildTask->pushLine(fileCursor, readBuffer);
				calculateOneHashStep(hash, (const unsigned char*)readBuffer->data(), readBuffer->size());
				fileCursor += lineSize + sizeof(uint16_t);
			}		

			unsigned char hash2[crypto_generichash_KEYBYTES];
			if (!fl->tryLockTimeout(mBlockPath, 100)) {
				throw LockException("couldn't lock file in time", mBlockPath.data());
			}
			auto fileStream = getOpenFile();			
			fileStream->read((char*)hash2, crypto_generichash_KEYBYTES);
			fl->unlock(mBlockPath);

			bool result = false;
			if (0 == sodium_memcmp(hash, hash2, crypto_generichash_KEYBYTES)) {
				result = true;
			}

			if (result) {
				return rebuildTask;
			}
			else {
				throw HashMismatchException(
					"block hash mismatch",
					memory::Block(sizeof hash, hash),
					memory::Block(sizeof hash2, hash2)
				);
			}
		}
			
		uint32_t Block::findLastBlockFileInFolder(std::string_view groupFolderPath)
		{
			std::filesystem::path groupFolder(groupFolderPath);
			uint32_t largestBlockNr = 0;
			std::regex isDatFile("blk(.*)\\.dat$");
			std::smatch match;
			for (const auto& entry : std::filesystem::directory_iterator(groupFolder)) {
				auto fileName = entry.path().filename().string();
				if (entry.is_regular_file() && std::regex_match(fileName, match, isDatFile)) {
					if (match.size() == 2) {
						auto blockNr = std::stoi(match[1].str());
						if (blockNr > largestBlockNr) {
							largestBlockNr = blockNr;
						}
					}
				}
			}
			return largestBlockNr;
		}

		void Block::calculateOneHashStep(unsigned char hash[crypto_generichash_KEYBYTES], const unsigned char* data, size_t dataSize)
		{
			crypto_generichash_state state;
			crypto_generichash_init(&state, nullptr, 0, crypto_generichash_BYTES);
			crypto_generichash_update(&state, hash, crypto_generichash_KEYBYTES);
			crypto_generichash_update(&state, data, dataSize);
			crypto_generichash_final(&state, hash, crypto_generichash_KEYBYTES);
		}


		// *********************** TASKS ***********************************

		BlockAppendLineTask::BlockAppendLineTask(std::shared_ptr<Block> block, std::vector<memory::ConstBlockPtr> lines)
			: task::CPUTask(ServerGlobals::g_WriteFileCPUScheduler), mTargetBlock(block), mLines(lines)
		{

		}

		int BlockAppendLineTask::run()
		{
#ifdef _UNI_LIB_DEBUG
			setName(mTargetBlock->getBlockPath().data());
#endif 
			mCursorPositions = mTargetBlock->appendLines(mLines);

			return 0;
		}


		RebuildBlockIndexTask::RebuildBlockIndexTask(std::shared_ptr<const gradido::blockchain::FileBased> blockchain)
			: task::CPUTask(ServerGlobals::g_CPUScheduler), mBlockchain(blockchain)
		{

		}

		int RebuildBlockIndexTask::run()
		{
			while (!mPendingFileCursorLine.empty()) 
			{
				std::pair<int32_t, std::shared_ptr<memory::Block>> fileCursorLine;
				if (!mPendingFileCursorLine.pop(fileCursorLine)) {
					throw std::runtime_error("don't get next file cursor line");
				}
				auto& serializedTransaction = fileCursorLine.second;
				deserialize::Context deserializer(serializedTransaction, deserialize::Type::CONFIRMED_TRANSACTION);
				deserializer.run();
				if (!deserializer.isConfirmedTransaction()) {
					throw InvalidGradidoTransaction("invalid transaction from block file while rebuilding block index", serializedTransaction);
				}
				lock();
				std::shared_ptr<gradido::blockchain::NodeTransactionEntry> transactionEntry = std::make_shared<gradido::blockchain::NodeTransactionEntry>(
					deserializer.getConfirmedTransaction(),
					mBlockchain,
					fileCursorLine.first
				);
				mTransactionEntries.push_back(transactionEntry);
				unlock();
			}
			return 0;
		}

		void RebuildBlockIndexTask::pushLine(int32_t fileCursor, std::shared_ptr<memory::Block> line)
		{
			mPendingFileCursorLine.push({ fileCursor, line });
		}
	}
}
