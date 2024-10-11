#ifndef __GRADIDO_NODE_MODEL_FILES_BLOCK_H
#define __GRADIDO_NODE_MODEL_FILES_BLOCK_H

#include "gradido_blockchain/lib/MultithreadQueue.h"

#include "../../lib/FuzzyTimer.h"

#include "../../task/CPUTask.h"

#include <fstream>

//! MAGIC NUMBER: use to check if a file is big enough to could contain a transaction
#define MAGIC_NUMBER_MINIMAL_TRANSACTION_SIZE 25

namespace controller {
	class AddressIndex;
}

namespace gradido {
	namespace blockchain {
		class NodeTransactionEntry;
		class FileBased;
	}
}

namespace model {
	namespace files {
		class RebuildBlockIndexTask;

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
			uint16_t readLine(uint32_t startReading, memory::BlockPtr buffer);
			std::shared_ptr<memory::Block> readLine(uint32_t startReading);

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

			// read whole file, validate hash
			std::shared_ptr<RebuildBlockIndexTask> rebuildBlockIndex(std::shared_ptr<gradido::blockchain::FileBased> blockchain);

			static uint32_t findLastBlockFileInFolder(std::string_view groupFolderPath);

		protected:
			//! \brief open file stream if not already open and set mCurrentFileSize via tellg, update mLastUsed
			//! \return block file stream
			std::shared_ptr<std::fstream> getOpenFile();
			//! \brief very expensive, read in whole file and calculate hash
			std::shared_ptr<memory::Block> calculateHash();

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

		
		class RebuildBlockIndexTask : public task::CPUTask
		{
		public:
			RebuildBlockIndexTask(std::shared_ptr<gradido::blockchain::FileBased> blockchain);
			const char* getResourceType() const { return "RebuildBlockIndexTask"; };

			int run();
			//! \param line will be moved
			void pushLine(int32_t fileCursor, std::shared_ptr<memory::Block> line);
			const std::list<std::shared_ptr<gradido::blockchain::NodeTransactionEntry>>& getTransactionEntries() const { return mTransactionEntries; }

			inline bool isPendingQueueEmpty() { return mPendingFileCursorLine.empty(); }

		protected:
			std::shared_ptr<gradido::blockchain::FileBased> mBlockchain;
			std::list<std::shared_ptr<gradido::blockchain::NodeTransactionEntry>> mTransactionEntries;
			MultithreadQueue<std::pair<int32_t, std::shared_ptr<memory::Block>>> mPendingFileCursorLine;
		};
	}
}

#endif // __GRADIDO_NODE_MODEL_FILES_BLOCK_H