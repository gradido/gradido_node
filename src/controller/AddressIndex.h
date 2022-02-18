#ifndef __GRADIDO_NODE_CONTROLLER_ADDRESS_INDEX_H
#define __GRADIDO_NODE_CONTROLLER_ADDRESS_INDEX_H

#include "ControllerBase.h"

#include <unordered_map>

#include "Poco/Path.h"
#include "Poco/AccessExpireCache.h"

#include "../model/files/AddressIndex.h"



namespace controller {

	/*!
	* @author Dario Rekowski
	* @date 2020-02-06
	*
	* @brief Manage address indices for user accounts
	*
	* Every user public key (account, address) gets an address index (uint32).\n
	* The block.index files contain this indices instead of the full public key (32 Byte) \n
	* to save storage space (3/4 saved) and more important memory and for faster look up. \n
	* It is number counting up which every new user public key, starting with 1.\n
	* Max user public keys per group: 4,294,967,295. \n
	*
	* Using folder with first byte of public key and filename with second byte of public key to keeping index file small.
	* File Path starting with first byte from account public key as folder and second byte as file name. \n
	* Example: Path for public key: 94937427d885fe93e22a76a6c839ebc4fdf4e5056012ee088cdebb89a24f778c\n
	* ./94/93.index
	*/

	class AddressIndex : public ControllerBase
	{
	public:
		AddressIndex(Poco::Path path, uint32_t lastIndex);

		//! \brief Get index from cache or if not in cache, loading file, maybe I/O read.
		//! \return Index or 0 if address didn't exist.
		uint32_t getIndexForAddress(const std::string& address);
		
		//! \brief Get or add index if not exist in cache or file, maybe I/O read.
		//! \param address User public key.
		//! \param lastIndex Last knowing index for group.
		//! \return Index for address.
		uint32_t getOrAddIndexForAddress(const std::string& address);

		std::vector<uint32_t> getOrAddIndicesForAddresses(std::vector<MemoryBin*>& publicKeys, bool clearMemoryBin = false);

		//! \brief Add index, maybe I/O read, I/O write if index is new.
		//! \param address User public key.
		//! \param index Index for address.
		//! \return False if index address already exist, else true.
		bool addAddressIndex(const std::string& address, uint32_t index);

		inline uint32_t getLastIndex() { Poco::ScopedLock<Poco::Mutex> lock(mWorkingMutex); return mLastIndex; }

		//! \brief take first hex sets from address as folder and file name
		//!
		//!  Example: Path for public key: 94937427d885fe93e22a76a6c839ebc4fdf4e5056012ee088cdebb89a24f778c\n
		//!  ./94/93.index
		static Poco::Path getAddressIndexFilePathForAddress(const std::string& address);
		static Poco::Path getAddressIndexFilePathForAddress(const MemoryBin* address);


	protected:
		//! \brief reading model::files::AddressIndex from cache or if not exist in cache, creating model::files::AddressIndex which load from file if exist
		//!
		//! Can need some time if file must at first load from disk, maybe I/O read.
		Poco::SharedPtr<model::files::AddressIndex> getAddressIndex(const std::string& address);
		Poco::SharedPtr<model::files::AddressIndex> getAddressIndex(const MemoryBin* address);

		Poco::Path mGroupPath;
		uint32_t   mLastIndex;
		Poco::AccessExpireCache<uint16_t, model::files::AddressIndex> mAddressIndicesCache;
	};
}

#endif //__GRADIDO_NODE_CONTROLLER_ADDRESS_INDEX_H
