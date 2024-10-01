#ifndef __GRADIDO_NODE_MODEL_FILES_DICTIONARY_H
#define __GRADIDO_NODE_MODEL_FILES_DICTIONARY_H

#include "../../lib/VirtualFile.h"
#include "../../lib/FuzzyTimer.h"
#include "../../task/SerializeToVFileTask.h"
#include "gradido_blockchain/data/AddressType.h"

#include <unordered_map>
#include <map>
#include <mutex>
#include <memory>

namespace model {
	namespace files {

		/*!
		* @author Dario Rekowski
		* @date 2020-01-29
		*
		* @brief File for storing strings accessable by index
		* 
		*
		*/
		class SuccessfullWrittenToFileCommand;

		class Dictionary : public task::ISerializeToVFile
		{
			friend SuccessfullWrittenToFileCommand;
		public:
			Dictionary(std::string_view fileName);
			~Dictionary();

			//! load from file, check hash, IO-read if exist, fileLock via FileLockManager
			//! \return true if loading from file worked or there isn't any file for reading
			//! \return false if error on reading file occured, that means index file must be created new
			bool init();
			//! write to file if current data set wasn't already written to file
			void exit();
			// remove file from hdd, clear map so basically start over fresh
			void reset();

			//! \brief Adding new index, account address public key pair into memory, use Poco::FastMutex::ScopedLock.
			//! \param address binary string
			//! \return True if ok or false if address already exist.
			bool add(const std::string& address, uint32_t index);
			//! \brief Get index for address from memory, use Poco::FastMutex::ScopedLock.
			//! \return Index or zero if address not found.
			uint32_t getIndexForString(const std::string &string) const;

			const std::string& getStringForIndex(uint32_t index) const;

			//! serialize address indices for writing with hdd write buffer task
			std::unique_ptr<VirtualFile> serialize();

			inline bool isFileWritten() const { std::lock_guard _lock(mFastMutex); return mFileWritten; }
			std::string getFileNameString() const { std::lock_guard _lock(mFastMutex); return mFileName; }
			size_t getIndexCount() const { std::lock_guard _lock(mFastMutex); return mStringIndices.size(); }

		protected:
			//! \brief Check if index file contains current indices (compare sizes), fileLock via FileLockManager, I/O read.
			//! TODO: check last hash
			//! \return True if calculated size from map entry count == file size, else return false.
			bool checkFile();

			void setFileWritten();

			//! \brief Calculate file size from map entry count + space for hash, use Poco::FastMutex::ScopedLock.
			//! \return Theoretical file site in bytes.
			size_t calculateFileSize();

			//! \brief Calculate file hash from map entrys sorted by indices.
			//! \return MemoryBin with hash, must be release after using by MemoryManager.
			static MemoryBin calculateHash(const std::map<uint32_t, std::string>& sortedMap);

			//! \brief Load index, public key pairs from file and check file integrity with sha256 hash at end of file, fileLock via FileLockManager, I/O read.
			//! \return False if file isn't valid or file couldn't be locked, else true if ok.
			bool loadFromFile();

			std::string mFileName;
			std::unordered_map<std::string, uint32_t> mStringIndices;
			//! Indicate if current index set is written to file (true) or not (false).
			bool mFileWritten;
			mutable std::mutex mFastMutex;
		};

		class SuccessfullWrittenToFileCommand : public task::Command
		{
		public:
			SuccessfullWrittenToFileCommand(std::shared_ptr<Dictionary> parent) : mParent(parent) {};
			int taskFinished(task::Task* task) { mParent->setFileWritten(); return 0; };

		protected:
			std::shared_ptr<Dictionary> mParent;
		};
	}
}

#endif //__GRADIDO_NODE_MODEL_FILES_DICTIONARY_H