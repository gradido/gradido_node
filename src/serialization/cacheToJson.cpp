#include "gradido_blockchain/export.h"
#include "gradido_blockchain/serialization/toJson.h"
#include "../cache/BlockIndex.h"

using namespace rapidjson;

namespace serialization {
	template<>
	Value toJson(const cache::BlockIndex& value, Document::AllocatorType& alloc)
	{
		return value.serializeToJson(alloc);
	}
}