#include "Group.h"
#include "sodium.h"
#include "../ServerGlobals.h"

#include "../SingletonManager/GroupManager.h"
#include "../SingletonManager/LoggerManager.h"

#include "../lib/DataTypeConverter.h"
#include "../lib/Exceptions.h"

#include "Block.h"

#include <inttypes.h>

#include "rapidjson/document.h"
#include "rapidjson/prettywriter.h"

using namespace rapidjson;

namespace controller {

	Group::Group(std::string groupAlias, Poco::Path folderPath)
		: mIotaMessageListener(nullptr), mGroupAlias(groupAlias),
		mFolderPath(folderPath), mGroupState(folderPath),
		mLastAddressIndex(0), mLastBlockNr(1), mLastTransactionId(0), mCachedBlocks(ServerGlobals::g_CacheTimeout),
		mCachedSignatures(static_cast<Poco::Timestamp::TimeDiff>(MAGIC_NUMBER_SIGNATURE_CACHE_MINUTES * 1000 * 60)),
		mCommunityServer(nullptr), mExitCalled(false)
	{
		mLastAddressIndex = mGroupState.getInt32ValueForKey("lastAddressIndex", mLastAddressIndex);
		mLastBlockNr = mGroupState.getInt32ValueForKey("lastBlockNr", mLastBlockNr);
		// TODO: sanity check, compare with block size
		mLastTransactionId = mGroupState.getInt32ValueForKey("lastTransactionId", mLastTransactionId);
		auto lastBlock = getBlock(mLastBlockNr);
		if (lastBlock->getBlockIndex()->getMaxTransactionNr() > mLastTransactionId) {
			updateLastTransactionId(lastBlock->getBlockIndex()->getMaxTransactionNr());
		}

		std::clog << "[Group " << groupAlias << "] "
			<< "loaded from state: last address index : " << std::to_string(mLastAddressIndex)
			<< ", last block nr: " << std::to_string(mLastBlockNr)
			<< ", last transaction id: " << std::to_string(mLastTransactionId)
			<< std::endl;
		mAddressIndex = new AddressIndex(folderPath, mLastAddressIndex);
	}

	Group::~Group()
	{
		if (mIotaMessageListener) {
			delete mIotaMessageListener;
		}
		mLastAddressIndex = mAddressIndex->getLastIndex();
		mGroupState.setKeyValue("lastAddressIndex", std::to_string(mLastAddressIndex));
		mGroupState.setKeyValue("lastBlockNr", std::to_string(mLastBlockNr));
		mGroupState.setKeyValue("lastTransactionId", std::to_string(mLastTransactionId));
		if (mCommunityServer) {
			delete mCommunityServer;
		}
	}

	bool Group::init()
	{
		fillSignatureCacheOnStartup();
		mIotaMessageListener = new iota::MessageListener("GRADIDO." + mGroupAlias);
		return true;
	}

	void Group::setListeningCommunityServer(Poco::URI uri)
	{
		Poco::ScopedLock<Poco::Mutex> lock(mWorkingMutex);
		if (mCommunityServer) {
			delete mCommunityServer;
		}
		mCommunityServer = new JsonRequest(uri.getHost(), uri.getPort(), uri.getPathAndQuery());
	}

	void Group::exit()
	{
		Poco::ScopedLock<Poco::Mutex> lock(mWorkingMutex);
		mExitCalled = true;
		if (mIotaMessageListener) {
			delete mIotaMessageListener;
			mIotaMessageListener = nullptr;
		}
		auto keys = mCachedBlocks.getAllKeys();
		for (auto it = keys.begin(); it != keys.end(); it++) {
			mCachedBlocks.get(*it)->exit();
		}
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
			Poco::ScopedLock<Poco::Mutex> lock(mWorkingMutex);
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

		Poco::ScopedLock<Poco::Mutex> lock(mWorkingMutex);
		mLastTransaction = newTransaction;
		auto block = getCurrentBlock();
		if (block.isNull()) {
			newTransaction->addError(new Error(__FUNCTION__, "didn't get valid block"));
			return false;
		}
		//TransactionEntry(uint64_t _transactionNr, std::string _serializedTransaction, Poco::DateTime received, uint16_t addressIndexCount = 2);
		Poco::SharedPtr<model::TransactionEntry> transactionEntry = new model::TransactionEntry(newTransaction, mAddressIndex);
		updateLastAddressIndex(mAddressIndex->getLastIndex());
		return block->pushTransaction(transactionEntry);

		// TODO: collect all indices for addresses in this block
		// after writing to block file, adding to block index
		// mAddressIndex.getOrAddIndexForAddress()

		//return true;
	}

	bool Group::addTransactionFromIota(Poco::AutoPtr<model::GradidoTransaction> newTransaction, uint32_t iotaMilestoneId, uint64_t iotaMilestoneTimestamp)
	{
		{
			if (!mWorkingMutex.tryLock(100)) {
				printf("[Group::addTransactionFromIota] try lock failed with transaction: %s\n", newTransaction->getJson().data());
			}
			else{
				mWorkingMutex.unlock();
			}
			
			Poco::ScopedLock<Poco::Mutex> lock(mWorkingMutex);
			if (mExitCalled) return false;
		}
		
		//printf("[Group::addTransactionFromIota] milestone: %d\n", iotaMilestoneId);
		model::TransactionValidationLevel level = model::TRANSACTION_VALIDATION_SINGLE;

		if (isTransactionAlreadyExist(newTransaction)) {
			printf("skip because transaction already exist\n");
			return false;
		}

		auto type = newTransaction->getTransactionBody()->getType();

		switch (type) {
		case model::TRANSACTION_CREATION:
			// check if address has get maximal 1.000 GDD creation in the month
			level = (model::TransactionValidationLevel)(level | model::TRANSACTION_VALIDATION_DATE_RANGE);
			//uint64_t sum = calculateCreationSum()
			break;
		case model::TRANSACTION_TRANSFER:
			// check if send address(es) have enough GDD
			level = (model::TransactionValidationLevel)(level | model::TRANSACTION_VALIDATION_SINGLE_PREVIOUS);
			// check if outbound transaction also exist
			if (newTransaction->getTransactionBody()->getTransfer()->isInbound()) {
				level = (model::TransactionValidationLevel)(level | model::TRANSACTION_VALIDATION_PAIRED);
				printf("is inbound\n");
			}
			else if(newTransaction->getTransactionBody()->getTransfer()->isOutbound()) {
				printf("is outbound\n");
			}
			break;
		}

		
		auto lastTransaction = getLastTransaction();
		uint32_t receivedSeconds = static_cast<uint32_t>(iotaMilestoneTimestamp);

		if (lastTransaction && lastTransaction->getReceivedSeconds() > receivedSeconds) {
			newTransaction->addError(new Error(__FUNCTION__, "previous transaction is younger than this transaction!"));
			return false;
		}

		Poco::AutoPtr<model::GradidoBlock> gradidoBlock(new model::GradidoBlock(
			receivedSeconds,
			std::string((const char*)&iotaMilestoneId, sizeof(uint32_t)),
			newTransaction,
			lastTransaction
		));

		
		// intern validation
		newTransaction->setGradidoBlock(gradidoBlock.get());
		printf("validate with level: %d\n", level);
		if (!newTransaction->validate(level)) {
			printf("failed: %s\n", newTransaction->getJson().data());
			return false;
		}

		Poco::ScopedLock<Poco::Mutex> lock(mWorkingMutex);
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
			// TODO: move call to better place
			updateLastAddressIndex(mAddressIndex->getLastIndex());
			addSignatureToCache(newTransaction);
			//std::clog << "add transaction: " << mLastTransaction->getID() << ", memo: " << newTransaction->getTransactionBody()->getMemo() << std::endl;
			printf("[%s] nr: %d, group: %s, messageId: %s, received: %d, transaction: %s",
				__FUNCTION__, mLastTransaction->getID(), mGroupAlias.data(), mLastTransaction->getMessageIdHex().data(), 
				mLastTransaction->getReceivedSeconds(), mLastTransaction->getGradidoTransaction()->getJson().data()
			);
			// say community server that a new transaction awaits him
			if (mCommunityServer) {
				ErrorList errors;
				auto result = mCommunityServer->PATCH(&errors);
				if (!errors.errorCount()) {
					StringBuffer buffer;
					PrettyWriter<StringBuffer> writer(buffer);
					result.Accept(writer);

					printf(buffer.GetString());
				}
				else {
					errors.printErrors();
				}
			}
		}
		return result;
	}

	uint64_t Group::calculateCreationSum(const std::string& address, int month, int year, Poco::DateTime received)
	{
		uint64_t sum = 0;
		std::vector<Poco::AutoPtr<model::GradidoBlock>> allTransactions;
		// received = max
		// received - 2 month = min
		Poco::DateTime searchDate = received;
		for (int i = 0; i < 3; i++) {
			auto transactions = findTransactions(address, searchDate.month(), searchDate.year());
			// https://stackoverflow.com/questions/201718/concatenating-two-stdvectors
			allTransactions.insert(
				allTransactions.end(),
				std::make_move_iterator(transactions.begin()),
				std::make_move_iterator(transactions.end())
			);
			searchDate -= Poco::Timespan(Poco::DateTime::daysOfMonth(searchDate.year(), searchDate.month()), 0, 0, 0, 0);
		}
		printf("[Group::calculateCreationSum] from group: %s\n", mGroupAlias.data());
		for (auto it = allTransactions.begin(); it != allTransactions.end(); it++) {
			auto body = (*it)->getGradidoTransaction()->getTransactionBody();
			if (body->getType() == model::TRANSACTION_CREATION) {
				auto creation = body->getCreation();
				auto targetDate = creation->getTargetDate();
				if (targetDate.month() != month || targetDate.year() != year) {
					continue;
				}
				printf("added from transaction: %d \n", (*it)->getID());
				sum += creation->getRecipiantAmount();
			}
		}
		return sum;
	}

	Poco::AutoPtr<model::GradidoBlock> Group::findLastTransaction(const std::string& address)
	{
		Poco::ScopedLock<Poco::Mutex> lock(mWorkingMutex);
		auto index = mAddressIndex->getIndexForAddress(address);
		if (!index) { return Poco::AutoPtr<model::GradidoBlock>(); }

		return Poco::AutoPtr<model::GradidoBlock>();
	}

	//std::vector<Poco::AutoPtr<model::GradidoBlock>> Group::findTransactions(uint64_t fromTransactionId)
	std::vector<std::string> Group::findTransactionsSerialized(uint64_t fromTransactionId)
	{
		std::vector<std::string> transactionsSerialized;
		uint64_t transactionIdCursor = fromTransactionId;
		// we cannot handle zero transaction id, starts with one,
		// but if someone ask for zero he gets all
		if (!transactionIdCursor) transactionIdCursor = 1;
		Poco::SharedPtr<Block> block;

		printf("[Group::findTransactionsSerialized] asked transactionId: %d, last: %d\n", fromTransactionId, mLastTransactionId);
		while (transactionIdCursor <= mLastTransactionId) {
			if (block.isNull() || !block->getBlockIndex()->hasTransactionNr(transactionIdCursor)) {
				block = getBlockContainingTransaction(transactionIdCursor);
			}
			if (block.isNull()) {
				throw std::runtime_error("[Group::findTransactionsSerialized] cannot find block for transaction id: " + std::to_string(transactionIdCursor));
			}
			std::string serializedTransaction;
			block->getTransaction(transactionIdCursor, serializedTransaction);
			transactionsSerialized.push_back(serializedTransaction);
			++transactionIdCursor;

		}
		return transactionsSerialized;
	}

	std::vector<Poco::AutoPtr<model::GradidoBlock>> Group::findTransactions(const std::string& address)
	{
		Poco::ScopedLock<Poco::Mutex> lock(mWorkingMutex);
		std::vector<Poco::AutoPtr<model::GradidoBlock>> transactions;
		auto gm = GroupManager::getInstance();

		auto index = mAddressIndex->getIndexForAddress(address);
		if (!index) { return transactions; }

		auto group = gm->findGroup(mGroupAlias);

		int blockCursor = mLastBlockNr;
		while (blockCursor > 0) {
			auto block = getBlock(blockCursor);
			auto blockIndex = block->getBlockIndex();
			auto transactionNrs = blockIndex->findTransactionsForAddress(index);
			if (transactionNrs.size()) {
				for (auto it = transactionNrs.begin(); it != transactionNrs.end(); it++) {
					std::string serializedTransaction;
					auto result = block->getTransaction(*it, serializedTransaction);
					if(!serializedTransaction.size()) {
						Poco::Logger& errorLog = LoggerManager::getInstance()->mErrorLogging;
						errorLog.fatal("corrupted data, get empty transaction for nr: %d, group: %s, result: %d",
							(int)*it, mGroupAlias, result);
						std::abort();
					}
					transactions.push_back(new model::GradidoBlock(serializedTransaction, group));
				}
			}
			blockCursor--;
		}

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
		Poco::ScopedLock<Poco::Mutex> lock(mWorkingMutex);
		std::vector<Poco::AutoPtr<model::GradidoBlock>> transactions;
		auto gm = GroupManager::getInstance();

		auto index = mAddressIndex->getIndexForAddress(address);
		if (!index) { return transactions; }
		
		auto group = gm->findGroup(mGroupAlias);

		int blockCursor = mLastBlockNr;
		while (blockCursor > 0) {
			auto block = getBlock(blockCursor);
			auto blockIndex = block->getBlockIndex();
			auto transactionNrs = blockIndex->findTransactionsForAddressMonthYear(index, year, month);
			if (transactionNrs.size()) {
				for (auto it = transactionNrs.begin(); it != transactionNrs.end(); it++) {
					std::string serializedTransaction;
					auto result = block->getTransaction(*it, serializedTransaction);
					if (!serializedTransaction.size()) {
						Poco::Logger& errorLog = LoggerManager::getInstance()->mErrorLogging;
						errorLog.fatal("corrupted data, get empty transaction for nr: %d, group: %s, result: %d",
							(int)*it, mGroupAlias, result);
						std::abort();
					}
					transactions.push_back(new model::GradidoBlock(serializedTransaction, group));
				}
			}
			else {
				auto newestYearMonth = blockIndex->getNewestYearMonth();
				if ((newestYearMonth.first < year) ||
				    (newestYearMonth.first == year && newestYearMonth.second < month)) {
					break;
				}
			}
			blockCursor--;
		}

		return transactions;
	}



	Poco::SharedPtr<Block> Group::getBlock(Poco::UInt32 blockNr)
	{
		auto block = mCachedBlocks.get(blockNr);
		if (!block.isNull()) {
			return block;
		}
		Poco::ScopedLock<Poco::Mutex> lock(mWorkingMutex);
		// Block(uint32_t blockNr, Poco::Path groupFolderPath, TaskObserver* taskObserver, const std::string& groupHash);
		block = new Block(blockNr, mFolderPath, &mTaskObserver, mGroupAlias);
		mCachedBlocks.add(blockNr, block);
		return block;
	}

	Poco::SharedPtr<Block> Group::getBlockContainingTransaction(uint64_t transactionId)
	{
		// find block by transactions id
		// approximately 300.000 transactions can be fitted in one block
		// for now search simply from last block
		// TODO: think of a better approach later, if more than 10 blocks a written
		int lastBlockNr = mLastBlockNr;
		Poco::SharedPtr<Block> block;
		do {
			if (lastBlockNr < 1) return nullptr;
			block = getBlock(lastBlockNr);
			if (block->getBlockIndex()->getMaxTransactionNr() < transactionId) {
				return nullptr;
			}
			lastBlockNr--;
		} while (!block->getBlockIndex()->hasTransactionNr(transactionId));

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

	bool Group::isTransactionAlreadyExist(Poco::AutoPtr<model::GradidoTransaction> transaction)
	{
		HalfSignature transactionSign(transaction);

		Poco::ScopedLock<Poco::FastMutex> _lock(mSignatureCacheMutex);
		if (mCachedSignatures.has(transactionSign)) {
			mCachedSignatures.get(transactionSign);
			return true;
		}
		return false;
	}

	void Group::addSignatureToCache(Poco::AutoPtr<model::GradidoTransaction> transaction)
	{
		Poco::ScopedLock<Poco::FastMutex> _lock(mSignatureCacheMutex);
		mCachedSignatures.add(HalfSignature(transaction), nullptr);
	}

	bool Group::isSignatureInCache(Poco::AutoPtr<model::GradidoTransaction> transaction)
	{
		Poco::ScopedLock<Poco::FastMutex> _lock(mSignatureCacheMutex);
		return mCachedSignatures.has(HalfSignature(transaction));
	}

	void Group::fillSignatureCacheOnStartup()
	{
		Poco::ScopedLock<Poco::FastMutex> _lock(mSignatureCacheMutex);
		int blockNr = mLastBlockNr;
		int transactionNr = mLastTransactionId;

		Poco::AutoPtr<model::GradidoBlock> transaction;
		Poco::DateTime border = Poco::DateTime() - Poco::Timespan(MAGIC_NUMBER_SIGNATURE_CACHE_MINUTES * 60, 0);

		do {
			if (transactionNr <= 0) break;
			if (blockNr <= 0) break;

			auto block = getBlock(blockNr);
			std::string serializedTransaction;
			auto getTransationResult = block->getTransaction(transactionNr, serializedTransaction);
			if (-2 == getTransationResult) {
				blockNr--;
				continue;
			}
			else if (0 != getTransationResult) {
				throw TransactionLoadingException(std::to_string(transactionNr), getTransationResult);
				//throw std::runtime_error("[Group::fillSignatureCacheOnStartup] couldn't load transaction: " + std::to_string(transactionNr));
			}
			transaction = new model::GradidoBlock(serializedTransaction, nullptr);
			auto sigPairs = transaction->getGradidoTransaction()->getProto().sig_map().sigpair();
			if (sigPairs.size()) {
				auto signature = DataTypeConverter::binToHex((const unsigned char*)sigPairs.Get(0).signature().data(), sigPairs.Get(0).signature().size());
				//printf("[Group::fillSignatureCacheOnStartup] add signature: %s\n", signature.data());
				HalfSignature transactionSign(sigPairs.Get(0).signature().data());
				mCachedSignatures.add(transactionSign, nullptr);
			}
			transactionNr--;
			//printf("compare received: %" PRId64 " with border : %" PRId64 "\n", transaction->getReceived().timestamp().raw(), border.timestamp().raw());
		} while (!transaction.isNull() && transaction->getReceived() > border);

	}
}
