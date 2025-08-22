#ifndef __GRADIDO_NODE_HIERO_MESSAGE_LISTENER_H
#define __GRADIDO_NODE_HIERO_MESSAGE_LISTENER_H

#include "gradido_blockchain/data/hiero/AccountId.h"

namespace hiero
{
	class MessageListener
	{
	public:
		MessageListener(const AccountId& topicId);
		~MessageListener();

		void start();
	protected:
		AccountId mTopicId;
	};
}

#endif //__GRADIDO_NODE_HIERO_MESSAGE_LISTENER_H