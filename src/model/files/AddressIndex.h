#ifndef __GRADIDO_NODE_MODEL_FILES_ADDRESS_INDEX_H
#define __GRADIDO_NODE_MODEL_FILES_ADDRESS_INDEX_H

#include "FileBase.h"
#include "../../lib/VirtualFile.h"
#include "../../lib/FuzzyTimer.h"
#include "../../task/SerializeToVFileTask.h"
#include "gradido_blockchain/data/AddressType.h"

#include "Poco/DateTime.h"
#include "Poco/SharedPtr.h"

#include <unordered_map>
#include <map>



namespace model {
	namespace files {

		/*!
		* @author Dario Rekowski
		* @date 2020-01-29
		*
		* @brief File for storing account address indices
		* 
		* Store indices for every account public key.\n
		*
		*/
		class SuccessfullWrittenToFileCommand;

		class AddressIndex : public FileBase, public task::ISerializeToVFile
		{
			friend SuccessfullWrittenToFileCommand;
		public:
			/*! @brief Container for address details which where together saved in address index file
			* TODO: Use this struct instead of only the index in map and in file, update all other code accordingly
			*/
			struct AddressDetails
			{
				AddressDetails(uint32_t _index, gradido::data::AddressType _type)
					: index(_index), type(_type) {}
				uint32_t index;
				gradido::data::AddressType type;
			};
			//! Load from file if exist, fileLock via FileLockManager, I/O read if exist.
			AddressIndex(Poco::Path filePath);
			~AddressIndex();

			//! \brief Adding new index, account address public key pair into memory, use Poco::FastMutex::ScopedLock.
			//! \param address binary string
			//! \return True if ok or false if address already exist.
			bool add(const std::string& address, uint32_t index);
			//! \brief Get index for address from memory, use Poco::FastMutex::ScopedLock.
			//! \return Index or zero if address not found.
			uint32_t getIndexForAddress(const std::string &address);

			//! \brief Write new index file if checkFile return true, use Poco::FastMutex::ScopedLock, fileLock via FileLockManager, I/O write.
			//! 
			//! Check if each index is unique
			//! Could need some time, calculate also sha256 hash for putting at end of file.
			//! Throw Poco::Exception if two addresses have the same index.
			void writeToFile();

			//! serialize address indices for writing with hdd write buffer task
			std::unique_ptr<VirtualFile> serialize();

			std::string getFileNameString();

			inline bool isFileWritten() { Poco::FastMutex::ScopedLock lock(mFastMutex); return mFileWritten; }

		protected:
			//! \brief Check if index file contains current indices (compare sizes), fileLock via FileLockManager, I/O read.
			//! \return True if calculated size from map entry count == file size, else return false.
			bool checkFile();

			void setFileWritten();

			//! \brief Calculate file size from map entry count + space for hash, use Poco::FastMutex::ScopedLock.
			//! \return Theoretical file site in bytes.
			size_t calculateFileSize();

			//! \brief Calculate file hash from map entrys sorted by indices.
			//! \return MemoryBin with hash, must be release after using by MemoryManager.
			static MemoryBin calculateHash(const std::map<int, std::string>& sortedMap);

			//! \brief Load index, public key pairs from file and check file integrity with sha256 hash at end of file, fileLock via FileLockManager, I/O read.
			//! \return False if file isn't valid or file couldn't be locked, else true if ok.
			bool loadFromFile();

			Poco::Path mFilePath;
			std::unordered_map<std::string, uint32_t> mAddressesIndices;
			//! Indicate if current index set is written to file (true) or not (false).
			bool mFileWritten;
		};

		class SuccessfullWrittenToFileCommand : public task::Command
		{
		public:
			SuccessfullWrittenToFileCommand(Poco::SharedPtr<AddressIndex> parent) : mParent(parent) {};
			int taskFinished(task::Task* task) { mParent->setFileWritten(); return 0; };

		protected:
			Poco::SharedPtr<AddressIndex> mParent;
		};
	}
}

#endif 