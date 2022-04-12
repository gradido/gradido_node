#ifndef __GRADIDO_NODE_MODEL_FILES_BLOCK_H
#define __GRADIDO_NODE_MODEL_FILES_BLOCK_H

#include "FileBase.h"

#include "Poco/Path.h"
#include "Poco/FileStream.h"
#include "Poco/Timestamp.h"
#include "Poco/Timer.h"

#include "gradido_blockchain/MemoryManager.h"
#include "gradido_blockchain/lib/MultithreadQueue.h"
#include "../../model/NodeTransactionEntry.h"

#include "../../lib/FuzzyTimer.h"

#include "../../task/CPUTask.h"

//! MAGIC NUMBER: use to check if a file is big enough to could contain a transaction
#define MAGIC_NUMBER_MINIMAL_TRANSACTION_SIZE 25

namespace controller {
	class AddressIndex;
}

namespace model {
	namespace files {
		class RebuildBlockIndexTask;

		class Block : public FileBase, public TimerCallback
		{
		public:
			Block(Poco::Path groupFolderPath, Poco::UInt32 blockNr);
			~Block();

			//! \brief close block file if not used > ServerGlobals::g_CacheTimeout  
			//!
			//! called from timer
			//void checkTimeout(Poco::Timer& timer);

			TimerReturn callFromTimer();
			const char* getResourceType() const { return "model::files::Block"; }

			//! \return size of line (without size field in file)
			Poco::UInt16 readLine(Poco::UInt32 startReading, std::string& strBuffer);
			std::unique_ptr<std::string> readLine(Poco::UInt32 startReading);			

			//! \brief call appendLines
			//! \return file cursor pos at start from this line in file (0 at start of file)
			//! \return -1 if block file couldn't locked
			//! \return -2 if uint32 data type isn't enough anymore
			Poco::Int32 appendLine(const std::string* line);
			std::vector<Poco::UInt32> appendLines(const std::vector<const std::string*>& lines);

			inline Poco::UInt32 getCurrentFileSize() { Poco::FastMutex::ScopedLock lock(mFastMutex); return mCurrentFileSize; }
			inline const std::string getBlockPath() const { return mBlockPath.toString(); }

			// very expensive, read in whole file and calculate hash
			bool validateHash();

			// read whole file, validate hash
			Poco::AutoPtr<RebuildBlockIndexTask> rebuildBlockIndex(Poco::SharedPtr<controller::AddressIndex> addressIndex);


		protected:
			//! \brief open file stream if not already open and set mCurrentFileSize via tellg, update mLastUsed
			//! \return block file stream
			Poco::SharedPtr<Poco::FileStream> getOpenFile();
			//! \brief very expensive, read in whole file and calculate hash
			MemoryBin* calculateHash();

			//Poco::Timer mTimer;

			Poco::Path   mBlockPath;
			Poco::UInt32 mBlockNr;
			Poco::UInt64 mLastWrittenTransactionNr;

			Poco::Timestamp mLastUsed;
			Poco::SharedPtr<Poco::FileStream>	mBlockFile;
			Poco::FastMutex mFastMutex;
			Poco::UInt32    mCurrentFileSize;

		};

		class BlockAppendLineTask : task::CPUTask
		{
		public:
			BlockAppendLineTask(Poco::SharedPtr<Block> block, std::vector<const std::string*> lines);

			const char* getResourceType() const { return "BlockAppendLineTask"; };

			int run();

		protected:
			Poco::SharedPtr<Block> mTargetBlock;
			std::vector<const std::string*> mLines;
			std::vector<Poco::UInt32> mCursorPositions;
		};

		
		class RebuildBlockIndexTask : public task::CPUTask
		{
		public:
			RebuildBlockIndexTask(Poco::SharedPtr<controller::AddressIndex> addressIndex);
			const char* getResourceType() const { return "RebuildBlockIndexTask"; };

			int run();
			//! \param line will be moved
			void pushLine(Poco::Int32 fileCursor, std::string line);
			const std::list<Poco::SharedPtr<model::NodeTransactionEntry>>& getTransactionEntries() const { return mTransactionEntries; }

			inline bool isPendingQueueEmpty() { return mPendingFileCursorLine.empty(); }

		protected:
			Poco::SharedPtr<controller::AddressIndex> mAddressIndex;
			std::list<Poco::SharedPtr<model::NodeTransactionEntry>> mTransactionEntries;
			MultithreadQueue<std::pair<Poco::Int32, std::string>> mPendingFileCursorLine;
		};
	}
}

#endif // __GRADIDO_NODE_MODEL_FILES_BLOCK_H