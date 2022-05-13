#include "ArchiveTransactionsOrdering.h"
#include "../SingletonManager/LoggerManager.h"
#include "Group.h"
#include "Poco/Util/ServerApplication.h"

namespace controller {
	ArchiveTransactionsOrdering::ArchiveTransactionsOrdering(Group* parentGroup)
		: mParentGroup(parentGroup)
	{

	}

	ArchiveTransactionsOrdering::~ArchiveTransactionsOrdering()
	{

	}

	void ArchiveTransactionsOrdering::addPendingTransaction(
		std::unique_ptr<model::gradido::GradidoTransaction> transaction,
		uint64_t transactionNr
	)
	{
		std::scoped_lock<std::shared_mutex> _lock(mPendingTransactionsMutex);
		auto result = mPendingTransactions.insert({ transactionNr, std::move(transaction) });
		if (!result.second) {
			throw ArchiveTransactionDoubletteException("transaction with this nr already exist", transactionNr);
		}
		condSignal();
	}

	int ArchiveTransactionsOrdering::ThreadFunction()
	{
		PendingTransactionsMap::iterator it;
		{
			std::scoped_lock<std::shared_mutex> _lock(mPendingTransactionsMutex);
			if (!mPendingTransactions.size()) return 0;
			it = mPendingTransactions.begin();
		}
		auto lastTransaction = mParentGroup->getLastTransaction();
		if (!lastTransaction.isNull()) {
			if (lastTransaction->getID() + 1 != it->first) {
				return 0;
			}
		}
		else if (it->first > 1) {
			return 0;
		}
		try {
			auto createdTimestamp = it->second->getTransactionBody()->getCreatedSeconds();
			mParentGroup->addTransaction(std::move(it->second), nullptr, createdTimestamp);
		}
		catch (GradidoBlockchainException& ex) {
			LoggerManager::getInstance()->mErrorLogging.critical("[ArchiveTransactionsOrdering] terminate with gradido blockchain exception: %s", ex.getFullString());
			Poco::Util::ServerApplication::terminate();
		}
		catch (Poco::Exception& ex) {
			LoggerManager::getInstance()->mErrorLogging.critical("[ArchiveTransactionsOrdering] terminate with poco exception: %s", ex.displayText());
			Poco::Util::ServerApplication::terminate();
		}
		catch (std::exception& ex) {
			LoggerManager::getInstance()->mErrorLogging.critical("[ArchiveTransactionsOrdering] terminate with exception: %s", std::string(ex.what()));
			Poco::Util::ServerApplication::terminate();
		}
		{
			std::scoped_lock<std::shared_mutex> _lock(mPendingTransactionsMutex);
			mPendingTransactions.erase(it);
		}

	}

	// *************** Exceptions ****************************
	ArchiveTransactionDoubletteException::ArchiveTransactionDoubletteException(const char* what, uint64_t transactionNr) noexcept
		: GradidoBlockchainException(what), mTransactionNr(transactionNr)
	{


	}

	std::string ArchiveTransactionDoubletteException::getFullString() const
	{
		std::string result = what();
		result += ", transaction nr: " + std::to_string(mTransactionNr);
		return result;
	}
}
