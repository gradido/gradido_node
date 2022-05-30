#include "MemoryAddressIndex.h"
#include "sodium.h"
#include "../ServerGlobals.h"
#include "../SingletonManager/LoggerManager.h"
#include "Group.h"
#include "../model/files/FileExceptions.h"

namespace controller {
	MemoryAddressIndex::MemoryAddressIndex(Poco::Path path, uint32_t lastIndex, Group* parent)
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

	std::vector<uint32_t> MemoryAddressIndex::getOrAddIndicesForAddresses(std::vector<MemoryBin*>& publicKeys, bool clearMemoryBin/* = false*/)
	{
		auto mm = MemoryManager::getInstance();
		std::vector<uint32_t> results;
		results.reserve(publicKeys.size());
		for (auto it = publicKeys.begin(); it != publicKeys.end(); it++) {
			results.push_back(getOrAddIndexForAddress(*(*it)->copyAsString().get()));
			if (clearMemoryBin) {
				mm->releaseMemory(*it);
			}
		}
		if (clearMemoryBin) {
			publicKeys.clear();
		}
		return results;
	}


}