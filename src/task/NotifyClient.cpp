#include "NotifyClient.h"
#include "../SingletonManager/LoggerManager.h"
#include "../ServerGlobals.h"

namespace task {
	NotifyClient::NotifyClient(client::Base* client, Poco::SharedPtr<model::gradido::ConfirmedTransaction> transactionBlock)
		: CPUTask(ServerGlobals::g_CPUScheduler), mClient(client), mTransactionBlock(transactionBlock)
	{

	}

	int NotifyClient::run()
	{
		try {
			mClient->notificateNewTransaction(mTransactionBlock);
		}
		catch (Poco::NullPointerException& ex) {
			LoggerManager::getInstance()->mErrorLogging.error("poco null pointer exception by calling notificateNewTransaction");
			throw;
		}
		return 0;
	}
}