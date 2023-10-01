#ifndef __GRADIDO_NODE_TASK_NOTIFY_CLIENT_H
#define __GRADIDO_NODE_TASK_NOTIFY_CLIENT_H

#include "CPUTask.h"
#include "../client/Base.h"

namespace task {
	class NotifyClient : public CPUTask
	{
	public:
		NotifyClient(client::Base* client, Poco::SharedPtr<model::gradido::ConfirmedTransaction> transactionBlock);

		const char* getResourceType() const { return "task::NotifyClient"; };
		int run();

	protected:
		client::Base* mClient;
		Poco::SharedPtr<model::gradido::ConfirmedTransaction> mTransactionBlock;
	};
}

#endif //__GRADIDO_NODE_TASK_NOTIFY_CLIENT_H