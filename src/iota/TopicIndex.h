#ifndef __GRADIDO_NODE_IOTA_TOPIC_INDEX_H
#define __GRADIDO_NODE_IOTA_TOPIC_INDEX_H

#include <string>
#include <memory>

namespace iota {

	/*!
	* @brief differentiate between alias and topic already in hex format
	*/
	class TopicIndex
	{
	public:
		TopicIndex(const std::string& topicIndex);

		// Copy Constructor
    	TopicIndex(const TopicIndex& other) : mTopicIndexHex(other.mTopicIndexHex) {}	
		~TopicIndex();
		inline const std::string& getTopicIndexHex() const {return mTopicIndexHex;}
		std::unique_ptr<std::string> getTopicIndexAlias() const;

		static bool isHex(const std::string& str);

	protected:
		std::string mTopicIndexHex;
	};
}

#endif //__GRADIDO_NODE_IOTA_TOPIC_INDEX_H