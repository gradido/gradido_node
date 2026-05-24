#ifndef GRADIDO_NODE_SERVER_JSON_RPC_FROM_JSON_H
#define GRADIDO_NODE_SERVER_JSON_RPC_FROM_JSON_H

#include "WireFilter.h"

#include <rapidjson/document.h>

#include <string>


namespace server::json_rpc {
  enum JsonParseResultType : uint8_t {
    Ok,
    Missing_Field,
    Null_Field,
    Invalid_Enum,
    Invalid_Field_Value,
    Invalid_Hex,
    Type_Mismatch,
    General_Error
  };

  struct JsonParseResult 
  {
      JsonParseResultType type;
      std::string error;
  };
  

  JsonParseResult fromJson(const rapidjson::Value& value, WireFilter& filter);
}

#endif // GRADIDO_NODE_SERVER_JSON_RPC_FROM_JSON_H