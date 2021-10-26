#include "ConfirmedMessagesCache.h"

namespace iota {
	ConfirmedMessagesCache::ConfirmedMessagesCache()
		// expire time in ms
		: mMessageCache(2 * 60 * 1000), mMaxMessageCount(0)
	{

	}

	ConfirmedMessagesCache::~ConfirmedMessagesCache()
	{
		auto memory = mMaxMessageCount * sizeof(MessageId);
		float memory_kByte = (float)memory / 1024.0f;
		printf("[~ConfirmedMessagesCache] max count: %d, memory consumption: %f kByte\n", mMaxMessageCount, memory_kByte);
	}

	ConfirmedMessagesCache* ConfirmedMessagesCache::getInstance()
	{
		static ConfirmedMessagesCache one;
		return &one;
	}

	bool ConfirmedMessagesCache::existInCache(const MessageId& messageId)
	{
		Poco::ScopedLock<Poco::FastMutex> _lock(mWorkMutex);
		if (mMessageCache.has(messageId)) {
			return true;
		}
		mMessageCache.add(messageId, messageId);
		if (mMessageCache.size() > mMaxMessageCount) {
			mMaxMessageCount = mMessageCache.size();
		}
		return false;
	}

	void ConfirmedMessagesCache::remove(const MessageId& messageId)
	{
		mMessageCache.remove(messageId);
	}
}