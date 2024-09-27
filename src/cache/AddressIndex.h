#ifndef __GRADIDO_NODE_CACHE_ADDRESS_INDEX_H
#define __GRADIDO_NODE_CACHE_ADDRESS_INDEX_H

#include "../model/files/AddressIndex.h"

#include <unordered_map>

namespace cache {
	class FileBased;

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
	* Max user public keys per group: 4,294,967,295. filesize 144 gb \n
	*
	* TODO: extend address index with first transaction nr where the address is involved
	*/


	class AddressIndex : public TimerCallback
	{
	public:
		AddressIndex(std::string_view path, gradido::blockchain::FileBased* parent);
		virtual ~AddressIndex();

		//! \return true if address indices could be loaded
		//! \return false if address index file was corrupted
		bool init();
		void exit();
		//! delete address index file instance and also file on disk
		void reset();

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

		inline uint32_t getLastIndex() { std::lock_guard _lock(mFastMutex); return mLastIndex; }

		//! \brief take first hex sets from address as folder and file name
		//!
		//!  Example: Path for public key: 94937427d885fe93e22a76a6c839ebc4fdf4e5056012ee088cdebb89a24f778c\n
		//!  ./94/93.index
		static std::string getAddressIndexFilePathForAddress(const std::string& address);

		TimerReturn callFromTimer();

	protected:
		std::string mGroupPath;
		uint32_t   mLastIndex;
		std::mutex mFastMutex;
		std::shared_ptr<model::files::AddressIndex> mAddressIndexFile;

		gradido::blockchain::FileBased* mParent;
		Timepoint mWaitingForNextFileWrite;
	};
}

#endif //__GRADIDO_NODE_CACHE_ADDRESS_INDEX_H
