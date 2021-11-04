#include "ConfirmedMessagesCache.h"
#include <iostream>

namespace iota {
	ConfirmedMessagesCache::ConfirmedMessagesCache()
		// expire time in micro seconds
		: mMessageCache(2 * 60 * 1000 * 1000), mMaxMessageCount(0)
	{

	}

	ConfirmedMessagesCache::~ConfirmedMessagesCache()
	{
		Poco::ScopedLock<Poco::FastMutex> _lock(mWorkMutex);
		auto memory = mMaxMessageCount * sizeof(MessageId);
		float memory_kByte = (float)memory / 1024.0f;
		std::clog << "[~ConfirmedMessagesCache] max count: " << mMaxMessageCount 
				  << ", memory consumption: " << memory_kByte << std::endl;
		//printf("[~ConfirmedMessagesCache] max count: %d, memory consumption: %f kByte\n", mMaxMessageCount, memory_kByte);
		mMessageCache.clear();
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
		Poco::ScopedLock<Poco::FastMutex> _lock(mWorkMutex);
		mMessageCache.remove(messageId);
	}
}
