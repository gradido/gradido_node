#ifndef __GRADIDO_NODE_CONTROLLER_ADDRESS_INDEX_H
#define __GRADIDO_NODE_CONTROLLER_ADDRESS_INDEX_H

#include "gradido_blockchain/lib/AccessExpireCache.h"

#include "ControllerBase.h"

#include "../model/files/AddressIndex.h"

#include <unordered_map>

namespace gradido {
	namespace blockchain {
		class FileBased;
	}
}

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
	* 
	* TODO: extend address index with first transaction nr where the address is involved
	*/

	

	class AddressIndex : public ControllerBase, public TimerCallback
	{
	public:
		AddressIndex(std::string_view path, uint32_t lastIndex, gradido::blockchain::FileBased* parent);
		~AddressIndex();

		//! \brief Get index from cache or if not in cache, loading file, maybe I/O read.
		//! \return Index or 0 if address didn't exist.
		virtual uint32_t getIndexForAddress(const std::string& address);
		
		//! \brief Get or add index if not exist in cache or file, maybe I/O read.
		//! \param address User public key.
		//! \param lastIndex Last knowing index for group.
		//! \return Index for address.
		virtual uint32_t getOrAddIndexForAddress(const std::string& address);

		virtual std::vector<uint32_t> getOrAddIndicesForAddresses(const std::vector<memory::ConstBlockPtr>& publicKeys);

		//! \brief Add index, maybe I/O read, I/O write if index is new.
		//! \param address User public key.
		//! \param index Index for address.
		//! \return False if index address already exist, else true.
		virtual bool addAddressIndex(const std::string& address, uint32_t index);

		inline uint32_t getLastIndex() { std::lock_guard _lock(mWorkingMutex); return mLastIndex; }

		//! \brief take first hex sets from address as folder and file name
		//!
		//!  Example: Path for public key: 94937427d885fe93e22a76a6c839ebc4fdf4e5056012ee088cdebb89a24f778c\n
		//!  ./94/93.index
		static std::string getAddressIndexFilePathForAddress(const std::string& address);

		TimerReturn callFromTimer();

	protected:
		//! \brief reading model::files::AddressIndex from cache or if not exist in cache, creating model::files::AddressIndex which load from file if exist
		//!
		//! Can need some time if file must at first load from disk, maybe I/O read.
		std::shared_ptr<model::files::AddressIndex> getAddressIndex(const std::string& address);

		std::string mGroupPath;
		uint32_t   mLastIndex;
		std::mutex mWorkMutex;
		AccessExpireCache<uint8_t, model::files::AddressIndex> mAddressIndicesCache;
		std::shared_ptr<model::files::AddressIndex> mAddressIndexFile;

		gradido::blockchain::FileBased *mParent;
		Timepoint mWaitingForNextFileWrite;
	};
}

#endif //__GRADIDO_NODE_CONTROLLER_ADDRESS_INDEX_H
