#include "Group.h"
#include "sodium.h"
#include "../ServerGlobals.h"

namespace controller {

	Group::Group(std::string base58Hash, Poco::Path folderPath)
		: mBase58GroupHash(base58Hash), 
		mFolderPath(folderPath), mAddressIndex(folderPath), mGroupState(folderPath),
		mLastKtoKey(0), mLastBlockNr(0), mCachedBlocks(ServerGlobals::g_CacheTimeout * 1000)
	{
		mLastKtoKey = mGroupState.getInt32ValueForKey("lastKtoIndex", mLastKtoKey);
		mLastBlockNr = mGroupState.getInt32ValueForKey("lastBlockNr", mLastBlockNr);

	}

	Group::~Group()
	{
		mGroupState.setKeyValue("lastKtoIndex", std::to_string(mLastKtoKey));
		mGroupState.setKeyValue("lastBlockNr", std::to_string(mLastBlockNr));
	}

	bool Group::addTransaction(Poco::AutoPtr<model::Transaction> newTransaction)
	{
		// intern validation
		if (!newTransaction->validate()) {
			return false;
		}

		// check previous transaction
		if (newTransaction->getID() > 1) 
		{
			Poco::FastMutex::ScopedLock lock(mWorkingMutex);
			if (mLastTransaction->getID() + 1 != newTransaction->getID()) {
				newTransaction->addError(new Error(__FUNCTION__, "Last transaction is not previous transaction"));
				return false;
			}
			// validate tx hash
			if (!newTransaction->validate(mLastTransaction)) {
				return false;
			}
		} 

		// validate specific	
		auto transactionBody = newTransaction->getTransactionBody();
		auto type = transactionBody->getType();
		model::TransactionBase* specificTransaction = nullptr;

		model::TransactionValidationLevel level = model::TRANSACTION_VALIDATION_SINGLE;


		switch (type) {
		case model::TRANSACTION_CREATION:
			// check if address has get maximal 1.000 GDD creation in the month 
			specificTransaction = transactionBody->getCreation();
			level = model::TRANSACTION_VALIDATION_DATE_RANGE;
			//uint64_t sum = calculateCreationSum()
			break;
		case model::TRANSACTION_TRANSFER:
			// check if send address(es) have enough GDD
			specificTransaction = transactionBody->getTransfer();
			level = model::TRANSACTION_VALIDATION_SINGLE_PREVIOUS;
			break;
		}
		if (!specificTransaction->validate(level)) {
			return false;
		}

		Poco::FastMutex::ScopedLock lock(mWorkingMutex);
		mLastTransaction = newTransaction;
		// TODO: collect all indices for addresses in this block
		// after writing to block file, adding to block index
		// mAddressIndex.getOrAddIndexForAddress()

		return true;
	}

	uint64_t Group::calculateCreationSum(const std::string& address, int month, int year)
	{
		uint64_t sum = 0;
		auto transactions = findTransactions(address, month, year);
		for (auto it = transactions.begin(); it != transactions.end(); it++) {
			auto body = (*it)->getTransactionBody();
			if (body->getType() == model::TRANSACTION_CREATION) {
				auto creation = body->getCreation();
				sum += creation->getReceiverAmount();
			}
		}
		return sum;
	}

	Poco::AutoPtr<model::Transaction> Group::findLastTransaction(const std::string& address)
	{
		Poco::FastMutex::ScopedLock lock(mWorkingMutex);
		auto index = mAddressIndex.getIndexForAddress(address);
		if (!index) { return Poco::AutoPtr<model::Transaction>(); }

		return Poco::AutoPtr<model::Transaction>();
	}

	std::vector<Poco::AutoPtr<model::Transaction>> Group::findTransactions(const std::string& address)
	{
		Poco::FastMutex::ScopedLock lock(mWorkingMutex);
		std::vector<Poco::AutoPtr<model::Transaction>> transactions;

		auto index = mAddressIndex.getIndexForAddress(address);
		if (!index) { return transactions; }

		return transactions;
	}

	std::vector<Poco::AutoPtr<model::Transaction>> Group::findTransactions(const std::string& address, int month, int year)
	{
		Poco::FastMutex::ScopedLock lock(mWorkingMutex);
		std::vector<Poco::AutoPtr<model::Transaction>> transactions;

		auto index = mAddressIndex.getIndexForAddress(address);
		if (!index) { return transactions; }
		int zahl = 0;



		return transactions;
	}



	Poco::SharedPtr<Block> Group::getBlock(Poco::UInt32 blockNr)
	{
		auto block = mCachedBlocks.get(blockNr);
		if (!block.isNull()) {
			return block;
		}
		Poco::FastMutex::ScopedLock lock(mWorkingMutex);
		block = new Block(blockNr);
		
	}

	

}