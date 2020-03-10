#ifndef __GRADIDO_NODE_MODEL_FILES_ADDRESS_INDEX_H
#define __GRADIDO_NODE_MODEL_FILES_ADDRESS_INDEX_H

#include "FileBase.h"
#include "../../SingletonManager/MemoryManager.h"
#include <unordered_map>



namespace model {
	namespace files {

		/*!
		* @author Dario Rekowski
		* @date 2020-01-29
		*
		* @brief File for storing account address indices
		* 
		* Store indices for every account public key.\n
		* File Path starting with first byte from account public key as folder and second byte as file name. \n
		* Example: Path for public key: 94937427d885fe93e22a76a6c839ebc4fdf4e5056012ee088cdebb89a24f778c\n
		* ./94/93.index
		*/

		class AddressIndex : public FileBase
		{
		public:
			//! Load from file if exist, fileLock via FileLockManager, I/O read if exist.
			AddressIndex(Poco::Path filePath);
			~AddressIndex();

			//! \brief Adding new index, account address public key pair into memory, use Poco::FastMutex::ScopedLock.
			//! \return True if ok or false if address already exist.
			bool addAddressIndex(const std::string& address, uint32_t index);
			//! \brief Get index for address from memory, use Poco::FastMutex::ScopedLock.
			//! \return Index or zero if address not found.
			uint32_t getIndexForAddress(const std::string &address);

			//! \brief Write new index file if checkFile return true, use Poco::FastMutex::ScopedLock, fileLock via FileLockManager, I/O write.
			//! 
			//! Could need some time, calculate also sha256 hash for putting at end of file.
			//! Throw Poco::Exception if two addresses have the same index.
			void safeExit();

		protected:
			//! \brief Check if index file contains current indices (compare sizes), fileLock via FileLockManager, I/O read.
			//! \return True if calculated size from map entry count == file size, else return false.
			bool checkFile();

			//! \brief Calculate file size from map entry count + space for hash, use Poco::FastMutex::ScopedLock.
			//! \return Theoretical file site in bytes.
			size_t calculateFileSize();

			//! \brief Calculate file hash from map entrys sorted by indices.
			//! \return MemoryBin with hash, must be release after using by MemoryManager.
			static MemoryBin* calculateHash(const std::map<int, std::string>& sortedMap);

			//! \brief Load index, public key pairs from file and check file integrity with sha256 hash at end of file, fileLock via FileLockManager, I/O read.
			//! \return False if file isn't valid or file couldn't be locked, else true if ok.
			bool loadFromFile();

			Poco::Path mFilePath;
			std::unordered_map<std::string, uint32_t> mAddressesIndices;
			//! Indicate if current index set is written to file true) or not (false).
			bool mFileWritten;
		};
	}
}

#endif 