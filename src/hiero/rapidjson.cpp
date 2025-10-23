#include "rapidjson.h"
#include "gradido_blockchain/GradidoBlockchainException.h"

using namespace rapidjson;

namespace hiero {
    gradido::data::Timestamp timestampFromJson(const rapidjson::Value& value)
    {
        assert(value.IsString());
        std::string str = std::string(value.GetString(), value.GetStringLength());
        auto splitPos = str.find('.');
        if (splitPos == std::string::npos) {
            throw GradidoNodeInvalidDataException("missing . in timestamp json string");
        }
        std::string secondsString = str.substr(0, splitPos);
        std::string nanosString = str.substr(splitPos + 1);
        auto seconds = strtoll(secondsString.c_str(), nullptr, 10);
        auto nanos = strtol(nanosString.c_str(), nullptr, 10);
        return gradido::data::Timestamp(seconds, nanos);
    }
    
}