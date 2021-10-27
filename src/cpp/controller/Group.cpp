#include "Group.h"
#include "sodium.h"
#include "../ServerGlobals.h"

#include "../SingletonManager/GroupManager.h"

#include "Block.h"

namespace controller {

	Group::Group(std::string groupAlias, Poco::Path folderPath)
		: mGroupAlias(groupAlias),
		mFolderPath(folderPath), mGroupState(folderPath),
		mLastAddressIndex(0), mLastBlockNr(1), mLastTransactionId(0), mCachedBlocks(ServerGlobals::g_CacheTimeout * 1000)
	{
		mLastAddressIndex = mGroupState.getInt32ValueForKey("lastAddressIndex", mLastAddressIndex);
		mLastBlockNr = mGroupState.getInt32ValueForKey("lastBlockNr", mLastBlockNr);
		mLastTransactionId = mGroupState.getInt32ValueForKey("lastTransactionId", mLastTransactionId);

		std::clog << "[Group " << groupAlias << "] " 
			<< "loaded from state: last address index : " << std::to_string(mLastAddressIndex)
			<< ", last block nr: " << std::to_string(mLastBlockNr) 
			<< ", last transaction id: " << std::to_string(mLastTransactionId)
			<< std::endl;
		mAddressIndex = new AddressIndex(folderPath, mLastAddressIndex);
	}

	Group::~Group()
	{
		mLastAddressIndex = mAddressIndex->getLastIndex();
		mGroupState.setKeyValue("lastAddressIndex", std::to_string(mLastAddressIndex));
		mGroupState.setKeyValue("lastBlockNr", std::to_string(mLastBlockNr));
		mGroupState.setKeyValue("lastTransactionId", std::to_string(mLastTransactionId));
	}

	void Group::updateLastAddressIndex(int lastAddressIndex)
	{
		mLastAddressIndex = lastAddressIndex;
		mGroupState.setKeyValue("lastAddressIndex", std::to_string(mLastAddressIndex));
	}
	void Group::updateLastBlockNr(int lastBlockNr)
	{	
		mLastBlockNr = lastBlockNr;
		mGroupState.setKeyValue("lastBlockNr", std::to_string(mLastBlockNr));
	}
	void Group::updateLastTransactionId(int lastTransactionId)
	{
		mLastTransactionId = lastTransactionId;
		mGroupState.setKeyValue("lastTransactionId", std::to_string(mLastTransactionId));
	}

	bool Group::addTransaction(Poco::AutoPtr<model::GradidoBlock> newTransaction)
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
		auto transactionBody = newTransaction->getGradidoTransaction()->getTransactionBody();
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
		auto block = getCurrentBlock();
		if (block.isNull()) {
			newTransaction->addError(new Error(__FUNCTION__, "didn't get valid block"));
			return false;
		}
		//TransactionEntry(uint64_t _transactionNr, std::string _serializedTransaction, Poco::DateTime received, uint16_t addressIndexCount = 2);
		Poco::SharedPtr<model::TransactionEntry> transactionEntry = new model::TransactionEntry(newTransaction, mAddressIndex);
		return block->pushTransaction(transactionEntry);

		// TODO: collect all indices for addresses in this block
		// after writing to block file, adding to block index
		// mAddressIndex.getOrAddIndexForAddress()

		//return true;
	}

	bool Group::addTransactionFromIota(Poco::AutoPtr<model::GradidoTransaction> newTransaction, uint32_t iotaMilestoneId, uint64_t iotaMilestoneTimestamp)
	{		
		printf("[Group::addTransactionFromIota] milestone: %d\n", iotaMilestoneId);
		model::TransactionValidationLevel level = model::TRANSACTION_VALIDATION_SINGLE;

		auto type = newTransaction->getTransactionBody()->getType();
		switch (type) {
		case model::TRANSACTION_CREATION:
			// check if address has get maximal 1.000 GDD creation in the month 
			level = model::TRANSACTION_VALIDATION_DATE_RANGE;
			//uint64_t sum = calculateCreationSum()
			break;
		case model::TRANSACTION_TRANSFER:
			// check if send address(es) have enough GDD
			level = model::TRANSACTION_VALIDATION_SINGLE_PREVIOUS;
			break;
		}
		
		// intern validation
		if (!newTransaction->validate(level)) {
			return false;
		}

		uint32_t receivedSeconds = static_cast<uint32_t>(iotaMilestoneTimestamp / 1000);
		Poco::AutoPtr<model::GradidoBlock> gradidoBlock(new model::GradidoBlock(
			receivedSeconds,
			std::string((const char*)&iotaMilestoneId, sizeof(uint32_t)),
			newTransaction,
			getLastTransaction()
		));

		Poco::FastMutex::ScopedLock lock(mWorkingMutex);
		auto block = getCurrentBlock();
		if (block.isNull()) {
			newTransaction->addError(new Error(__FUNCTION__, "didn't get valid block"));
			return false;
		}
		//TransactionEntry(uint64_t _transactionNr, std::string _serializedTransaction, Poco::DateTime received, uint16_t addressIndexCount = 2);
		Poco::SharedPtr<model::TransactionEntry> transactionEntry = new model::TransactionEntry(gradidoBlock, mAddressIndex);
		bool result = block->pushTransaction(transactionEntry);
		if (result) {
			mLastTransaction = gradidoBlock;
			updateLastTransactionId(gradidoBlock->getID());
		}
		return result;
	}

	uint64_t Group::calculateCreationSum(const std::string& address, int month, int year)
	{
		uint64_t sum = 0;
		auto transactions = findTransactions(address, month, year);
		for (auto it = transactions.begin(); it != transactions.end(); it++) {
			auto body = (*it)->getGradidoTransaction()->getTransactionBody();
			if (body->getType() == model::TRANSACTION_CREATION) {
				auto creation = body->getCreation();
				sum += creation->getRecipiantAmount();
			}
		}
		return sum;
	}

	Poco::AutoPtr<model::GradidoBlock> Group::findLastTransaction(const std::string& address)
	{
		Poco::FastMutex::ScopedLock lock(mWorkingMutex);
		auto index = mAddressIndex->getIndexForAddress(address);
		if (!index) { return Poco::AutoPtr<model::GradidoBlock>(); }

		return Poco::AutoPtr<model::GradidoBlock>();
	}

	std::vector<Poco::AutoPtr<model::GradidoBlock>> Group::findTransactions(const std::string& address)
	{
		Poco::FastMutex::ScopedLock lock(mWorkingMutex);
		std::vector<Poco::AutoPtr<model::GradidoBlock>> transactions;

		auto index = mAddressIndex->getIndexForAddress(address);
		if (!index) { return transactions; }

		return transactions;
	}

	Poco::AutoPtr<model::GradidoBlock> Group::getLastTransaction()
	{
		if (!mLastTransaction.isNull()) {
			return mLastTransaction;
		}
		if (!mLastTransactionId) {
			return nullptr;
		}
		auto block = getBlock(mLastBlockNr);
		std::string serializedTransaction;
		if (block->getTransaction(mLastTransactionId, serializedTransaction)) {
			return nullptr;
		}
		// call group manager to get shared ptr for this group, maybe replace with auto ptr
		// but the good thing is the group cache will be updated
		mLastTransaction = new model::GradidoBlock(serializedTransaction, GroupManager::getInstance()->findGroup(mGroupAlias));
		return mLastTransaction;
	}

	std::vector<Poco::AutoPtr<model::GradidoBlock>> Group::findTransactions(const std::string& address, int month, int year)
	{
		Poco::FastMutex::ScopedLock lock(mWorkingMutex);
		std::vector<Poco::AutoPtr<model::GradidoBlock>> transactions;

		auto index = mAddressIndex->getIndexForAddress(address);
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
		// Block(uint32_t blockNr, Poco::Path groupFolderPath, TaskObserver* taskObserver, const std::string& groupHash);
		block = new Block(blockNr, mFolderPath, &mTaskObserver, mGroupAlias);
		mCachedBlocks.add(blockNr, block);
		return block;
	}

	Poco::SharedPtr<Block> Group::getCurrentBlock()
	{
		auto block = getBlock(mLastBlockNr);
		if (!block.isNull() && block->hasSpaceLeft()) {
			return block;
		}
		// call update mLastBlockNr variable and write update to state db
		updateLastBlockNr(mLastBlockNr+1);
		block = getBlock(mLastBlockNr);
		if (!block.isNull() && block->hasSpaceLeft()) {
			return block;
		}
		return nullptr;
	}
	

}