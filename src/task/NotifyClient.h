#ifndef __GRADIDO_NODE_TASK_NOTIFY_CLIENT_H
#define __GRADIDO_NODE_TASK_NOTIFY_CLIENT_H

#include "CPUTask.h"
#include "../client/Base.h"

namespace task {
	class NotifyClient : public CPUTask
	{
	public:
		NotifyClient(std::shared_ptr<client::Base> client, std::shared_ptr<const gradido::data::ConfirmedTransaction> confirmedTransaction);

		const char* getResourceType() const override { return "task::NotifyClient"; };
		int run() override;

	protected:
		std::shared_ptr<client::Base> mClient;
		std::shared_ptr<const gradido::data::ConfirmedTransaction> mConfirmedTransaction;
	};
}

#endif //__GRADIDO_NODE_TASK_NOTIFY_CLIENT_H