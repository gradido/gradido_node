#include "GroupRegisterGroup.h"

#include "gradido_blockchain/GradidoBlockchainException.h"
#include "ControllerExceptions.h"
#include "../ServerGlobals.h"
#include "Poco/File.h"

namespace controller {
	GroupRegisterGroup::GroupRegisterGroup()
		: Group(GROUP_REGISTER_GROUP_ALIAS, Poco::Path(Poco::Path(ServerGlobals::g_FilesPath, ""), Poco::Path(GROUP_REGISTER_GROUP_ALIAS, "")), 0)
	{
		Poco::File file(mFolderPath);
		if (!file.exists()) {
			file.createDirectory();
		}
	}

	GroupRegisterGroup::~GroupRegisterGroup()
	{

	}

	GroupEntry GroupRegisterGroup::findGroup(const std::string& groupAlias)
	{
		auto it = mRegisteredGroups.find(groupAlias);
		if (it == mRegisteredGroups.end()) {
			throw GroupNotFoundException("couldn't find group on register group blockchain", groupAlias);
		}
		return it->second;
	}

	uint32_t GroupRegisterGroup::generateUniqueCoinColor()
	{
		std::set<uint32_t> coinColors;
		std::for_each(mRegisteredGroups.begin(), mRegisteredGroups.end(), [&](auto it) {
			coinColors.insert(it.second.coinColor);
		});
		uint32_t result = rand();
		int maxTryCount = 1000;
		while (coinColors.find(result) != coinColors.end() && maxTryCount > 0) {
			result = rand();
			maxTryCount--;
		}
		if (maxTryCount <= 0) {
			throw std::runtime_error("error with rand, cannot find new unique coin color");
		}
		return result;
	}

	bool GroupRegisterGroup::isCoinColorUnique(uint32_t coinColor)
	{
		for (auto it = mRegisteredGroups.begin(); it != mRegisteredGroups.end(); it++) {
			if (it->second.coinColor == coinColor) {
				return false;
			}
		}
		return true;
	}

	bool GroupRegisterGroup::isGroupAliasUnique(const std::string& groupAlias)
	{
		auto it = mRegisteredGroups.find(groupAlias);
		if (it == mRegisteredGroups.end()) {
			return true;
		}
		return false;
	}

	bool GroupRegisterGroup::addTransaction(std::unique_ptr<model::gradido::GradidoTransaction> newTransaction, const MemoryBin* messageId, uint64_t iotaMilestoneTimestamp)
	{
		Poco::ScopedLock _lock(mWorkingMutex);
		if (!newTransaction->getTransactionBody()->isGlobalGroupAdd()) {
			throw InvalidTransactionTypeOnBlockchain(
				"non global group add don't belong to GroupRegister blockchain",
				newTransaction->getTransactionBody()->getTransactionType()
			);
		}
		auto result = Group::addTransaction(std::move(newTransaction), messageId, iotaMilestoneTimestamp);
		if (result) {
			auto transaction = getLastTransaction();
			auto globalGroupAdd = transaction->getGradidoTransaction()->getTransactionBody()->getGlobalGroupAdd();
			mRegisteredGroups.insert({ globalGroupAdd->getGroupAlias(), GroupEntry(transaction->getID(), globalGroupAdd->getCoinColor()) });
			return true;
		}
		return false;
	}

	void GroupRegisterGroup::fillSignatureCacheOnStartup()
	{
		auto transactions = getAllTransactions();
		// for signature fill up on startup 
		Poco::Timestamp border = Poco::Timestamp() - Poco::Timespan(MAGIC_NUMBER_SIGNATURE_CACHE_MINUTES * 60, 0);

		for (auto it = transactions.begin(); it != transactions.end(); it++) 
		{
			auto transaction = std::make_unique<model::gradido::GradidoBlock>((*it)->getSerializedTransaction());
			if (!transaction->getGradidoTransaction()->getTransactionBody()->isGlobalGroupAdd()) {
				throw InvalidTransactionTypeOnBlockchain(
					"a non global group add found on GroupRegister blockchain", 
					transaction->getGradidoTransaction()->getTransactionBody()->getTransactionType()
				);
			}
			auto globalGroupAdd = transaction->getGradidoTransaction()->getTransactionBody()->getGlobalGroupAdd();
			// fill mRegisteredGroups
			mRegisteredGroups.insert({ globalGroupAdd->getGroupAlias(), GroupEntry(transaction->getID(), globalGroupAdd->getCoinColor()) });
			// from Group::fillSignatureCacheOnStartup
			if (transaction->getReceivedAsTimestamp() > border) {
				mCachedSignatureTransactionNrs.add(HalfSignature(transaction->getGradidoTransaction()), transaction->getID());
			}
		}
	}
}