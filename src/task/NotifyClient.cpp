#include "NotifyClient.h"
#include "../ServerGlobals.h"
#include "gradido_blockchain/GradidoBlockchainException.h"

#include "loguru/loguru.hpp"

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
		catch (GradidoBlockchainException& ex) {
			LOG_F(ERROR, "uncatched gradido blockchain exception: %s", ex.getFullString().data());
			throw;
		}
		return 0;
	}
}