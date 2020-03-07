#include "AddressIndex.h"
#include "sodium.h"
#include "../ServerGlobals.h"

namespace controller {
	AddressIndex::AddressIndex(Poco::Path path)
		: mGroupPath(path), mAddressIndicesCache(ServerGlobals::g_CacheTimeout * 1000)
	{

	}

	bool AddressIndex::addAddressIndex(const std::string& address, uint32_t index)
	{
		auto addressIndex = getAddressIndex(address);
		if (addressIndex.isNull()) {
			return false;
		}
		return addressIndex->addAddressIndex(address, index);
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

	uint32_t AddressIndex::getOrAddIndexForAddress(const std::string& address, uint32_t lastIndex)
	{
		auto addressIndex = getAddressIndex(address);
		assert(addressIndex.isNull());
		auto index = addressIndex->getIndexForAddress(address);
		if (!index) {
			index = lastIndex + 1;
			addressIndex->addAddressIndex(address, index);
		}
		return index;
	}

	Poco::SharedPtr<model::files::AddressIndex> AddressIndex::getAddressIndex(const std::string& address)
	{
		uint16_t firstBytes = 0;
		memcpy(&firstBytes, address.data(), sizeof(uint16_t));
		auto entry = mAddressIndicesCache.get(firstBytes);

		if (entry.isNull()) {
			Poco::Path addressIndexPath(mGroupPath);
			char firstBytesHex[5]; memset(firstBytesHex, 0, 5);
			sodium_bin2hex(firstBytesHex, 5, (const unsigned char*)address.data(), 2);
			//printf("bytes %x\n", firstBytes);
			std::string firstBytesString = firstBytesHex;
			addressIndexPath.pushDirectory("pubkeys_" + firstBytesString.substr(0, 2));
			std::string file = "_" + firstBytesString.substr(2, 2);
			file += ".index";
			addressIndexPath.append(file);
			//printf("index path: %s\n", addressIndexPath.toString().data());
			// pubkeys_<first hash byte hex>
			// _<second hash byte hex>.index
			Poco::SharedPtr<model::files::AddressIndex> newAddressIndex(new model::files::AddressIndex(addressIndexPath));
			mAddressIndicesCache.add(firstBytes, newAddressIndex);
			return newAddressIndex;
		}
		
		return entry;
	}
}