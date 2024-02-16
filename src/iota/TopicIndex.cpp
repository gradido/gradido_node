#include "TopicIndex.h"
#include "gradido_blockchain/lib/DataTypeConverter.h"
#include "Poco/RegularExpression.h"

using namespace DataTypeConverter;

namespace iota
{
    TopicIndex::TopicIndex(const std::string& topicIndex) 
    {
        if(isHex(topicIndex)) {
            mTopicIndexHex = topicIndex;
        } else {
            mTopicIndexHex = DataTypeConverter::binToHex(topicIndex);
        }
    }

    TopicIndex::~TopicIndex()
    {
       
    }

	std::unique_ptr<std::string> TopicIndex::getTopicIndexAlias() const
    {
        return hexToBinString(mTopicIndexHex);
    }

    bool TopicIndex::isHex(const std::string& str) {
        static Poco::RegularExpression regex("^[0-9a-fA-F]+$");
        return regex.match(str);
    }

} // namespace iota
