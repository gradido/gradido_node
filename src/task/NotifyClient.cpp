#include "NotifyClient.h"
#include "../SingletonManager/LoggerManager.h"
#include "../ServerGlobals.h"

namespace task {
	NotifyClient::NotifyClient(client::Base* client, std::shared_ptr<gradido::data::ConfirmedTransaction> confirmedTransaction)
		: CPUTask(ServerGlobals::g_CPUScheduler), mClient(client), mConfirmedTransaction(confirmedTransaction)
	{

	}

	int NotifyClient::run()
	{
		try {
			mClient->notificateNewTransaction(*mConfirmedTransaction);
		}
		catch (Poco::NullPointerException& ex) {
			LoggerManager::getInstance()->mErrorLogging.error("poco null pointer exception by calling notificateNewTransaction");
			throw;
		}
		return 0;
	}
}