#ifndef __GRADIDO_NODE_CONTROLLER_MEMORY_ADDRESS_INDEX_H
#define __GRADIDO_NODE_CONTROLLER_MEMORY_ADDRESS_INDEX_H

#include "ControllerBase.h"

#include <unordered_map>

#include "Poco/Path.h"
#include "Poco/AccessExpireCache.h"

#include "AddressIndex.h"

namespace controller {

	/*!
	* @author einhornimmond
	* @date 2022-05-27
	*	
	*/

	class Group;

	class MemoryAddressIndex : public AddressIndex
	{
	public:
		MemoryAddressIndex(Poco::Path path, uint32_t lastIndex, Group* parent);

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

	protected:
		std::unordered_map<std::string, uint32_t> mAddressIndices;
		
	};
}

#endif //__GRADIDO_NODE_CONTROLLER_MEMORY_ADDRESS_INDEX_H
