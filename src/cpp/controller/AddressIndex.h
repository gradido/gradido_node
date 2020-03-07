#ifndef __GRADIDO_NODE_CONTROLLER_ADDRESS_INDEX_H
#define __GRADIDO_NODE_CONTROLLER_ADDRESS_INDEX_H

#include "ControllerBase.h"

#include <unordered_map>

#include "Poco/Path.h"
#include "Poco/AccessExpireCache.h"

#include "../model/files/AddressIndex.h"

namespace controller {
	class AddressIndex : public ControllerBase
	{
	public:
		AddressIndex(Poco::Path path);
		
		uint32_t getIndexForAddress(const std::string& address);
		uint32_t getOrAddIndexForAddress(const std::string& address, uint32_t lastIndex);
		bool addAddressIndex(const std::string& address, uint32_t index);


	protected:
		Poco::SharedPtr<model::files::AddressIndex> getAddressIndex(const std::string& address);

		Poco::Path mGroupPath;
		Poco::AccessExpireCache<uint16_t, model::files::AddressIndex> mAddressIndicesCache;
	};
}

#endif __GRADIDO_NODE_CONTROLLER_ADDRESS_INDEX_H