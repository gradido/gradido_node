#ifndef __GRADIDO_NODE_MODEL_FILES_BLOCK_H
#define __GRADIDO_NODE_MODEL_FILES_BLOCK_H

#include "FileBase.h"

#include "Poco/Path.h"
#include "Poco/FileStream.h"
#include "Poco/Timestamp.h"

#include "../../SingletonManager/TimeoutManager.h"
#include "../../SingletonManager/MemoryManager.h"

#include "../../task/CPUTask.h"

namespace model {
	namespace files {
		class Block : public FileBase, public ITimeout
		{
		public:
			Block(Poco::Path groupFolderPath, Poco::UInt32 blockNr);
			~Block();

			//! \brief close block file if not used > ServerGlobals::g_CacheTimeout  
			//!
			//! called from timeout manager
			void checkTimeout();

			//! \return -1 error locking file for reading
			//! \return -2 error invalid size (greater as file size)
			//! \return 0 ok
			int readLine(Poco::UInt32 startReading, std::string& resultString);

			//! \brief call appendLines
			//! \return file cursor pos at start from this line in file (0 at start of file)
			//! \return -1 if block file couldn't locked
			//! \return -2 if uint32 data type isn't enough anymore
			Poco::Int32 appendLine(const std::string& line);
			std::vector<Poco::UInt32> appendLines(const std::vector<std::string>& lines);

			inline Poco::UInt32 getCurrentFileSize() { Poco::FastMutex::ScopedLock lock(mFastMutex); return mCurrentFileSize; }
			inline const std::string getBlockPath() const { return mBlockPath.toString(); }

			// very expensive, read in whole file and calculate hash
			bool validateHash();

		protected:
			Poco::SharedPtr<Poco::FileStream> getOpenFile();
			// very expensive, read in whole file and calculate hash
			MemoryBin* calculateHash();

			Poco::Path   mBlockPath;
			Poco::UInt32 mBlockNr;

			Poco::Timestamp mLastUsed;
			Poco::SharedPtr<Poco::FileStream>	mBlockFile;
			Poco::FastMutex mFastMutex;
			Poco::UInt32    mCurrentFileSize;

		};

		class BlockAppendLineTask : UniLib::controller::CPUTask
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