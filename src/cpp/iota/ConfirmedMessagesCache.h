#ifndef GRADIDO_NODE_IOTA_CONFIRMED_MESSAGES_CACHE
#define GRADIDO_NODE_IOTA_CONFIRMED_MESSAGES_CACHE

#include "Message.h"
#include "Poco/ExpireCache.h"

namespace iota
{
	class ConfirmedMessagesCache
	{
	public:
		~ConfirmedMessagesCache();
		static ConfirmedMessagesCache* getInstance();

		// return true if exist in cache, false if not, will be added if not exist
		bool existInCache(const MessageId& messageId);

		void remove(const MessageId& messageId);

	protected:
		ConfirmedMessagesCache();
		Poco::ExpireCache<MessageId, MessageId> mMessageCache;
		Poco::FastMutex mWorkMutex;
		int mMaxMessageCount;
	};
}

#endif