#include "AddressIndex.h"
#include "sodium.h"
#include "../ServerGlobals.h"

namespace controller {
	AddressIndex::AddressIndex(Poco::Path path, uint32_t lastIndex)
		: mGroupPath(path), mLastIndex(lastIndex), mAddressIndicesCache(ServerGlobals::g_CacheTimeout)
	{

	}

	bool AddressIndex::addAddressIndex(const std::string& address, uint32_t index)
	{
		auto addressIndex = getAddressIndex(address);
		if (addressIndex.isNull()) {
			return false;
		}
		mWorkingMutex.lock();
		mLastIndex = index;
		mWorkingMutex.unlock();

		return addressIndex->add(address, index);
	}

	uint32_t AddressIndex::getIndexForAddress(const std::string &address)
	{
		auto addressIndex = getAddressIndex(address);
		if (addressIndex.isNull()) {
			return 0;
		}

		auto index = addressIndex->getIndexForAddress(address);
		return index;
	}

	
	uint32_t AddressIndex::getOrAddIndexForAddress(const std::string& address)
	{
		auto addressIndex = getAddressIndex(address);
		assert(!addressIndex.isNull());
		auto index = addressIndex->getIndexForAddress(address);
		if (!index) {
			index = ++mLastIndex;
			addressIndex->add(address, index);
		}
		return index;
	}

	std::vector<uint32_t> AddressIndex::getOrAddIndicesForAddresses(std::vector<MemoryBin*>& publicKeys, bool clearMemoryBin/* = false*/)
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

	Poco::SharedPtr<model::files::AddressIndex> AddressIndex::getAddressIndex(const std::string& address)
	{
		uint16_t firstBytes = 0;
		memcpy(&firstBytes, address.data(), sizeof(uint16_t));
		auto entry = mAddressIndicesCache.get(firstBytes);

		if (entry.isNull()) {
			
			Poco::Path addressIndexPath(mGroupPath);
			addressIndexPath.append(getAddressIndexFilePathForAddress(address));
			
			Poco::SharedPtr<model::files::AddressIndex> newAddressIndex(new model::files::AddressIndex(addressIndexPath));
			mAddressIndicesCache.add(firstBytes, newAddressIndex);
			return newAddressIndex;
		}
		
		return entry;
	}

	Poco::SharedPtr<model::files::AddressIndex> AddressIndex::getAddressIndex(const MemoryBin* address)
	{
		uint16_t firstBytes = 0;
		memcpy(&firstBytes, address, sizeof(uint16_t));
		auto entry = mAddressIndicesCache.get(firstBytes);

		if (entry.isNull()) {
			Poco::Path addressIndexPath(mGroupPath);
			addressIndexPath.append(getAddressIndexFilePathForAddress(address));

			Poco::SharedPtr<model::files::AddressIndex> newAddressIndex(new model::files::AddressIndex(addressIndexPath));
			mAddressIndicesCache.add(firstBytes, newAddressIndex);
			return newAddressIndex;
		}
		return nullptr;
	}

	Poco::Path AddressIndex::getAddressIndexFilePathForAddress(const std::string& address)
	{
		Poco::Path addressIndexPath;
		char firstBytesHex[5]; memset(firstBytesHex, 0, 5);
		sodium_bin2hex(firstBytesHex, 5, (const unsigned char*)address.data(), 2);
		//printf("bytes %x\n", firstBytes);
		std::string firstBytesString = firstBytesHex;
		addressIndexPath.pushDirectory("pubkeys");
		addressIndexPath.pushDirectory("_" + firstBytesString.substr(0, 2));
		std::string file = "_" + firstBytesString.substr(2, 2);
		file += ".index";
		addressIndexPath.append(file);

		return addressIndexPath;
	}

	Poco::Path AddressIndex::getAddressIndexFilePathForAddress(const MemoryBin* address)
	{
		Poco::Path addressIndexPath;
		char firstBytesHex[5]; memset(firstBytesHex, 0, 5);
		sodium_bin2hex(firstBytesHex, 5, *address, 2);
		std::string firstBytesString = firstBytesHex;
		addressIndexPath.pushDirectory("pubkeys");
		addressIndexPath.pushDirectory("_" + firstBytesString.substr(0, 2));
		std::string file = "_" + firstBytesString.substr(2, 2);
		file += ".index";
		addressIndexPath.append(file);

		return addressIndexPath;
	}

}