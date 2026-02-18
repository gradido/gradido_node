#ifndef __GRADIDO_NODE_MODEL_FILES_BLOCK_H
#define __GRADIDO_NODE_MODEL_FILES_BLOCK_H

#include "gradido_blockchain/lib/MultithreadQueue.h"

#include "../../lib/FuzzyTimer.h"

#include "../../task/CPUTask.h"

#include "gradido_protobuf_zig.h"

#include <sodium.h>

#include <fstream>
#include <memory>

//! MAGIC NUMBER: use to check if a file is big enough to could contain a transaction
#define MAGIC_NUMBER_MINIMAL_TRANSACTION_SIZE 25

namespace cache {
	class BlockIndex;
}


namespace controller {
	class AddressIndex;
}

namespace gradido {
	namespace blockchain {
		class NodeTransactionEntry;
		class FileBased;
	}
}

namespace memory {
	class Block;
	using BlockPtr = std::shared_ptr<Block>;
}

namespace task {
	class RebuildBlockIndexTask;
}

namespace model {
	namespace files {
		class RebuildBlockIndexTask;

		class IBlockBufferRead
		{
		public:
			virtual void finishedLine(uint16_t memStart, uint16_t size, int32_t fileCursor) = 0;
			// will be called after last line was finished
			virtual void flush() = 0;
		};

		class Block : public TimerCallback
		{
		public:
			Block(std::string_view groupFolderPath, uint32_t blockNr);
			~Block();

			//! \brief close block file if not used > ServerGlobals::g_CacheTimeout  
			//!
			//! called from timer
			//void checkTimeout(Poco::Timer& timer);

			TimerReturn callFromTimer();
			const char* getResourceType() const { return "model::files::Block"; }

			//! \return size of line (without size field in file)
			uint16_t readLine(uint32_t startReading, memory::BlockPtr* buffer);
			std::shared_ptr<memory::Block> readLine(uint32_t startReading);
			// read whole file, validate hash
			bool readBuffered(grdu_memory* alloc, IBlockBufferRead* callback);

			//! \brief call appendLines
			//! \return file cursor pos at start from this line in file (0 at start of file)
			//! \return -1 if block file couldn't locked
			//! \return -2 if uint32 data type isn't enough anymore
			int32_t appendLine(memory::ConstBlockPtr line);
			std::vector<uint32_t> appendLines(const std::vector<memory::ConstBlockPtr>& lines);

			inline uint32_t getCurrentFileSize() { std::scoped_lock _lock(mFastMutex); return mCurrentFileSize; }
			inline std::string getBlockPath() const { return mBlockPath; }

			// very expensive, read in whole file and calculate hash
			bool validateHash();

			static uint32_t findLastBlockFileInFolder(std::string_view groupFolderPath);

		protected:
			//! \brief open file stream if not already open and set mCurrentFileSize via tellg, update mLastUsed
			//! \return block file stream
			std::shared_ptr<std::fstream> getOpenFile();
			//! \brief very expensive, read in whole file and calculate hash
			std::shared_ptr<memory::Block> calculateHash();

			void calculateOneHashStep(unsigned char hash[crypto_generichash_KEYBYTES], const unsigned char* data, size_t dataSize);

			//Poco::Timer mTimer;

			std::string   mBlockPath;
			uint32_t mBlockNr;
			uint64_t mLastWrittenTransactionNr;

			Timepoint mLastUsed;
			std::shared_ptr<std::fstream>	mBlockFile;
			std::mutex mFastMutex;
			uint32_t  mCurrentFileSize;

		};

		class BlockAppendLineTask : task::CPUTask
		{
		public:
			BlockAppendLineTask(std::shared_ptr<Block> block, std::vector<memory::ConstBlockPtr> lines);

			const char* getResourceType() const { return "BlockAppendLineTask"; };

			int run();

		protected:
			std::shared_ptr<Block> mTargetBlock;
			std::vector<memory::ConstBlockPtr> mLines;
			std::vector<uint32_t> mCursorPositions;
		};

		
	}
}

#endif // __GRADIDO_NODE_MODEL_FILES_BLOCK_H