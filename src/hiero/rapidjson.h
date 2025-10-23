#ifndef __GRADIDO_NODE_HIERO_RAPIDJSON_H
#define __GRADIDO_NODE_HIERO_RAPIDJSON_H

#include "rapidjson/document.h"
#include "gradido_blockchain/data/Timestamp.h"

namespace hiero {
    gradido::data::Timestamp timestampFromJson(const rapidjson::Value& value);
}

#endif // __GRADIDO_NODE_HIERO_RAPIDJSON_H