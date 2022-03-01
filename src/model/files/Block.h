#ifndef __GRADIDO_NODE_MODEL_FILES_BLOCK_H
#define __GRADIDO_NODE_MODEL_FILES_BLOCK_H

#include "FileBase.h"

#include "Poco/Path.h"
#include "Poco/FileStream.h"
#include "Poco/Timestamp.h"
#include "Poco/Timer.h"

#include "gradido_blockchain/MemoryManager.h"

#include "../../lib/FuzzyTimer.h"

#include "../../task/CPUTask.h"

//! MAGIC NUMBER: use to check if a file is big enough to could contain a transaction
#define MAGIC_NUMBER_MINIMAL_TRANSACTION_SIZE 25

namespace model {
	namespace files {
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

			//! \return -1 error locking file for reading
			//! \return -2 error invalid size (greater as file size)
			//! \return -3 error if startReading is greater than mCurrentFileSize
			//! \return 0 ok
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
			std::streampos  mCurrentFileCursorReadPosition;

		};

		class BlockAppendLineTask : task::CPUTask
		{
		public:
			BlockAppendLineTask(Poco::SharedPtr<Block> block, std::vector<std::string> lines);

			const char* getResourceType() const { return "BlockAppendLineTask"; };

			int run();

		protected:
			Poco::SharedPtr<Block> mTargetBlock;
			std::vector<std::string> mLines;
			std::vector<Poco::UInt32> mCursorPositions;
		};
	}
}

#endif // __GRADIDO_NODE_MODEL_FILES_BLOCK_H