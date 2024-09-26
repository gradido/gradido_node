#include "../blockchain/FileBased.h"

#include "MemoryAddressIndex.h"
#include "sodium.h"
#include "../ServerGlobals.h"
#include "../model/files/FileExceptions.h"

namespace controller {
	MemoryAddressIndex::MemoryAddressIndex(std::string_view path, uint32_t lastIndex, gradido::blockchain::FileBased* parent)
		: AddressIndex(path, lastIndex, parent)
	{

	}

	bool MemoryAddressIndex::addAddressIndex(const std::string& address, uint32_t index)
	{
		mAddressIndices.insert({ address, index });
		
		mWorkingMutex.lock();
		mLastIndex = index;
		mWorkingMutex.unlock();

		return true;
	}

	uint32_t MemoryAddressIndex::getIndexForAddress(const std::string& address)
	{
		auto it = mAddressIndices.find(address);
		if (it != mAddressIndices.end()) {
			return it->second;
		}
		return 0;
	}


	uint32_t MemoryAddressIndex::getOrAddIndexForAddress(const std::string& address)
	{
		auto index = getIndexForAddress(address);
		
		if (!index) {
			index = ++mLastIndex;
			addAddressIndex(address, index);
		}
		return index;
	}

	std::vector<uint32_t> MemoryAddressIndex::getOrAddIndicesForAddresses(const std::vector<memory::BlockPtr>& publicKeys)
	{
		std::vector<uint32_t> results;
		results.reserve(publicKeys.size());
		for (auto it = publicKeys.begin(); it != publicKeys.end(); it++) {
			results.push_back(getOrAddIndexForAddress((*it)->copyAsString()));
		}
		return results;
	}


}