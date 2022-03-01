#include "Group.h"
#include "RemoteGroup.h"
#include "sodium.h"
#include "../ServerGlobals.h"

#include "../SystemExceptions.h"
#include "ControllerExceptions.h"

#include "../SingletonManager/GroupManager.h"
#include "../SingletonManager/LoggerManager.h"

#include "gradido_blockchain/lib/DataTypeConverter.h"
#include "gradido_blockchain/model/TransactionFactory.h"

#include "Block.h"

#include <inttypes.h>

#include "rapidjson/document.h"
#include "rapidjson/prettywriter.h"

using namespace rapidjson;

namespace controller {

	Group::Group(std::string groupAlias, Poco::Path folderPath, uint32_t coinColor)
		: mIotaMessageListener(nullptr), mGroupAlias(groupAlias),
		mFolderPath(folderPath), mGroupState(folderPath),
		mLastAddressIndex(0), mLastBlockNr(1), mLastTransactionId(0), mCoinColor(coinColor), mCachedBlocks(ServerGlobals::g_CacheTimeout * 1000),
		mCachedSignatures(static_cast<Poco::Timestamp::TimeDiff>(MAGIC_NUMBER_SIGNATURE_CACHE_MINUTES * 1000 * 60)),
		mMessageIdTransactionNrCache(ServerGlobals::g_CacheTimeout * 1000),
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
		mCommunityServer = new JsonRequest(uri);
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

	bool Group::addTransaction(std::unique_ptr<model::gradido::GradidoTransaction> newTransaction, const MemoryBin* messageId, uint64_t iotaMilestoneTimestamp)
	{
		{
			if (!mWorkingMutex.tryLock(100)) {
				// TODO: use exception
				printf("[Group::addTransactionFromIota] try lock failed with transaction: %s\n", newTransaction->toJson().data());
			}
			else{
				mWorkingMutex.unlock();
			}
			
			Poco::ScopedLock<Poco::Mutex> lock(mWorkingMutex);
			if (mExitCalled) return false;
		}
		
		if (isTransactionAlreadyExist(newTransaction.get())) {
			throw GradidoBlockchainTransactionAlreadyExistException("[Group::addTransaction] skip because already exist");
		}
				
		auto lastTransaction = getLastTransaction();
		uint64_t id = 1;
		if (lastTransaction) {
			id = lastTransaction->getID() + 1;
		}
		auto newGradidoBlock = TransactionFactory::createGradidoBlock(std::move(newTransaction), id, iotaMilestoneTimestamp, messageId);
		if (newGradidoBlock->getReceived() < lastTransaction->getReceived()) {
			throw BlockchainOrderException("previous transaction is younger");
		}
		// calculate final balance
		calculateFinalBalance(newGradidoBlock);

		// intern validation
		model::gradido::TransactionValidationLevel level = (model::gradido::TransactionValidationLevel)(
			model::gradido::TRANSACTION_VALIDATION_SINGLE |
			model::gradido::TRANSACTION_VALIDATION_SINGLE_PREVIOUS |
			model::gradido::TRANSACTION_VALIDATION_DATE_RANGE |
			model::gradido::TRANSACTION_VALIDATION_PAIRED
			);

		printf("validate with level: %d\n", level);
		auto otherGroup = newGradidoBlock->getGradidoTransaction()->getTransactionBody()->getOtherGroup();
		model::IGradidoBlockchain* otherBlockchain = nullptr;
		std::unique_ptr<RemoteGroup> mOtherGroupRemote;
		if (otherGroup.size()) {
			auto otherGroupPtr = GroupManager::getInstance()->findGroup(otherGroup);
			if (!otherGroupPtr.isNull()) {
				otherBlockchain = otherGroupPtr.get();
			}
			else {
				// we don't usually listen on this group blockchain
				mOtherGroupRemote = std::make_unique<RemoteGroup>(otherGroup);
				otherBlockchain = mOtherGroupRemote.get();
			}
		}
		// if something went wrong, it throws an exception
		newGradidoBlock->validate(level, this, otherBlockchain);		

		Poco::ScopedLock<Poco::Mutex> lock(mWorkingMutex);
		auto block = getCurrentBlock();
		if (block.isNull()) {
			throw BlockNotLoadedException("don't get a valid current block", mGroupAlias, mLastBlockNr);
		}
		//TransactionEntry(uint64_t _transactionNr, std::string _serializedTransaction, Poco::DateTime received, uint16_t addressIndexCount = 2);
		Poco::SharedPtr<model::NodeTransactionEntry> transactionEntry = new model::NodeTransactionEntry(newGradidoBlock, mAddressIndex);
		bool result = block->pushTransaction(transactionEntry);
		if (result) {
			mLastTransaction = newGradidoBlock;
			updateLastTransactionId(newGradidoBlock->getID());
			// TODO: move call to better place
			updateLastAddressIndex(mAddressIndex->getLastIndex());
			addSignatureToCache(newGradidoBlock);
			mMessageIdTransactionNrCacheMutex.lock();
			iota::MessageId mid; mid.fromMemoryBin(messageId);
			mMessageIdTransactionNrCache.add(mid, mLastTransaction->getID());
			mMessageIdTransactionNrCacheMutex.unlock();
			
			//std::clog << "add transaction: " << mLastTransaction->getID() << ", memo: " << newTransaction->getTransactionBody()->getMemo() << std::endl;
			printf("[%s] nr: %d, group: %s, messageId: %s, received: %d, transaction: %s",
				__FUNCTION__, mLastTransaction->getID(), mGroupAlias.data(), mLastTransaction->getMessageIdHex().data(), 
				mLastTransaction->getReceived(), mLastTransaction->getGradidoTransaction()->toJson().data()
			);
			// say community server that a new transaction awaits him
			if (mCommunityServer) {
				Value params(kObjectType);
				auto alloc = mCommunityServer->getJsonAllocator();
				auto transactionBase64 = DataTypeConverter::binToBase64(mLastTransaction->getSerialized());
				params.AddMember("transactionBase64", Value(transactionBase64.get()->data(), alloc), alloc);
				try {
					auto result = mCommunityServer->postRequest(params);
					if (result.IsObject()) {
						StringBuffer buffer;
						PrettyWriter<StringBuffer> writer(buffer);
						result.Accept(writer);

						printf(buffer.GetString());
					}
				}
				catch (RapidjsonParseErrorException& ex) {
					printf("[Group::addTransaction] Result Json Exception: %s\n", ex.getFullString().data());
				}
				catch (Poco::Exception& ex) {
					printf("[Group::addTransaction] Poco Exception: %s\n", ex.displayText().data());
				}
				
			}
		}
		return result;
	}

	uint64_t Group::calculateCreationSum(const std::string& address, int month, int year, Poco::DateTime received)
	{
		uint64_t sum = 0;
		std::vector<Poco::SharedPtr<model::NodeTransactionEntry>> allTransactions;
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
			auto gradidoBlock = std::make_unique<model::gradido::GradidoBlock>((*it)->getSerializedTransaction());
			auto body = gradidoBlock->getGradidoTransaction()->getTransactionBody();
			if (body->getTransactionType() == model::gradido::TRANSACTION_CREATION) {
				auto creation = body->getCreationTransaction();
				auto targetDate = creation->getTargetDate();
				if (targetDate.month() != month || targetDate.year() != year) {
					continue;
				}
				printf("added from transaction: %d \n", gradidoBlock->getID());
				sum += creation->getAmount();
			}
		}
		// TODO: if user has moved from another blockchain, get also creation transactions from origin group, recursive
		return sum;
	}

	Poco::AutoPtr<model::gradido::GradidoBlock> Group::findLastTransaction(const std::string& address)
	{
		Poco::ScopedLock<Poco::Mutex> lock(mWorkingMutex);
		auto index = mAddressIndex->getIndexForAddress(address);
		if (!index) { return Poco::AutoPtr<model::gradido::GradidoBlock>(); }

		return Poco::AutoPtr<model::gradido::GradidoBlock>();
	}

	//std::vector<Poco::AutoPtr<model::GradidoBlock>> Group::findTransactions(uint64_t fromTransactionId)
	std::vector<Poco::SharedPtr<model::TransactionEntry>> Group::findTransactionsFromXToLast(uint64_t fromTransactionId)
	{
		std::vector<Poco::SharedPtr<model::TransactionEntry>> transactionEntries;
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
			auto transactionEntry = block->getTransaction(transactionIdCursor);
			// make copy from serialized transactions
			transactionEntries.push_back(transactionEntry);
			++transactionIdCursor;

		}
		return transactionEntries;
	}

	std::vector<Poco::SharedPtr<model::NodeTransactionEntry>> Group::findTransactions(const std::string& address)
	{
		Poco::ScopedLock<Poco::Mutex> lock(mWorkingMutex);
		std::vector<Poco::SharedPtr<model::NodeTransactionEntry>> transactions;
		
		auto index = mAddressIndex->getIndexForAddress(address);
		if (!index) { return transactions; }

		int blockCursor = mLastBlockNr;
		while (blockCursor > 0) {
			auto block = getBlock(blockCursor);
			auto blockIndex = block->getBlockIndex();
			auto transactionNrs = blockIndex->findTransactionsForAddress(index);
			if (transactionNrs.size()) {
				for (auto it = transactionNrs.begin(); it != transactionNrs.end(); it++) {
					
					auto result = block->getTransaction(*it);
					if(!result->getSerializedTransaction()->size()) {
						Poco::Logger& errorLog = LoggerManager::getInstance()->mErrorLogging;
						errorLog.fatal("corrupted data, get empty transaction for nr: %d, group: %s, result: %d",
							(int)*it, mGroupAlias, result);
						std::abort();
					}
					transactions.push_back(result);
				}
			}
			blockCursor--;
		}

		return transactions;
	}

	Poco::SharedPtr<model::gradido::GradidoBlock> Group::getLastTransaction()
	{
		if (mLastTransaction) {
			return mLastTransaction;
		}
		if (!mLastTransactionId) {
			return nullptr;
		}
		auto block = getBlock(mLastBlockNr);
		// throw an exception if transaction wasn't found
		auto transaction = block->getTransaction(mLastTransactionId);
		
		mLastTransaction = new model::gradido::GradidoBlock(transaction->getSerializedTransaction());
		return mLastTransaction;
	}

	Poco::SharedPtr<model::TransactionEntry> Group::getTransactionForId(uint64_t transactionId)
	{
		auto blockNr = mLastBlockNr;
		Poco::SharedPtr<controller::Block> block;
		do {
			block = getBlock(blockNr);
			blockNr--;
		} while (transactionId < block->getBlockIndex()->getMinTransactionNr() && blockNr > 0);
		
		return block->getTransaction(transactionId);
	}

	Poco::SharedPtr<model::TransactionEntry> Group::findByMessageId(const MemoryBin* messageId, bool cachedOnly/* = true*/)
	{
		Poco::ScopedLock _lock(mMessageIdTransactionNrCacheMutex);
		iota::MessageId mid;
		mid.fromMemoryBin(messageId);
		auto it = mMessageIdTransactionNrCache.get(mid);
		if(!it.isNull()) {
			return getTransactionForId(*it.get());
		}
		if (cachedOnly) {
			return nullptr;
		}
		auto blockNr = mLastBlockNr;
		do {
			auto block = getBlock(blockNr);
			auto blockIndex = block->getBlockIndex();
			for (int i = blockIndex->getMinTransactionNr(); i <= blockIndex->getMaxTransactionNr(); i++) {
				auto transactionEntry = block->getTransaction(i);
				auto transaction = std::make_unique<model::gradido::GradidoBlock>(transactionEntry->getSerializedTransaction());
				iota::MessageId transactionMessageId;
				auto messageIdMemoryBin = transaction->getMessageId();
				transactionMessageId.fromMemoryBin(messageIdMemoryBin);
				MemoryManager::getInstance()->releaseMemory(messageIdMemoryBin);
				if (transactionMessageId == mid) {
					return transactionEntry;
				}
			}
			blockNr--;
		} while (blockNr > 0);

	}

	std::vector<Poco::SharedPtr<model::NodeTransactionEntry>> Group::findTransactions(const std::string& address, int month, int year)
	{
		Poco::ScopedLock<Poco::Mutex> lock(mWorkingMutex);
		std::vector<Poco::SharedPtr<model::NodeTransactionEntry>>  transactions;
		
		auto index = mAddressIndex->getIndexForAddress(address);
		if (!index) { return transactions; }		

		int blockCursor = mLastBlockNr;
		while (blockCursor > 0) {
			auto block = getBlock(blockCursor);
			auto blockIndex = block->getBlockIndex();
			auto transactionNrs = blockIndex->findTransactionsForAddressMonthYear(index, year, month);
			if (transactionNrs.size()) {
				// sort nr ascending to possible speed up read from block file
				// TODO: check if there not already perfectly sorted
				std::sort(std::begin(transactionNrs), std::end(transactionNrs));
				for (auto it = transactionNrs.begin(); it != transactionNrs.end(); it++) {
					auto result = block->getTransaction(*it);
					if (!result->getSerializedTransaction()->size()) {
						Poco::Logger& errorLog = LoggerManager::getInstance()->mErrorLogging;
						errorLog.fatal("corrupted data, get empty transaction for nr: %d, group: %s, result: %d",
							(int)*it, mGroupAlias, result);
						std::abort();
					}
					transactions.push_back(result);
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

		return std::move(transactions);
	}

	std::vector<Poco::SharedPtr<model::TransactionEntry>> Group::getAllTransactions(std::function<bool(model::TransactionEntry*)> filter/* = nullptr*/)
	{
		std::vector<Poco::SharedPtr<model::TransactionEntry>> result;
		auto lastBlock = getBlock(mLastBlockNr);
		result.reserve(lastBlock->getBlockIndex()->getMaxTransactionNr());
		int blockCursor = 1;
		while (blockCursor <= mLastBlockNr)
		{
			auto block = getBlock(blockCursor);
			auto blockIndex = block->getBlockIndex();
			for (int i = blockIndex->getMinTransactionNr(); i <= blockIndex->getMaxTransactionNr(); i++) {
				auto transaction = block->getTransaction(i);
				if (!filter || filter(transaction)) {
					result.push_back(block->getTransaction(i));
				}
			}
			blockCursor++;
		}
		return std::move(result);
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
		// if return nullptr, it seems no space is left on disk or in memory
		return nullptr;
	}

	bool Group::isTransactionAlreadyExist(const model::gradido::GradidoTransaction* transaction) 
	{
		HalfSignature transactionSign(transaction);

		Poco::ScopedLock<Poco::FastMutex> _lock(mSignatureCacheMutex);
		if (mCachedSignatures.has(transactionSign)) {
			mCachedSignatures.get(transactionSign);
			return true;
		}
		return false;
	}

	void Group::addSignatureToCache(Poco::SharedPtr<model::gradido::GradidoBlock> gradidoBlock)
	{
		Poco::ScopedLock<Poco::FastMutex> _lock(mSignatureCacheMutex);
		mCachedSignatures.add(HalfSignature(gradidoBlock->getGradidoTransaction()), nullptr);
	}

	bool Group::isSignatureInCache(Poco::AutoPtr<model::gradido::GradidoTransaction> transaction)
	{
		Poco::ScopedLock<Poco::FastMutex> _lock(mSignatureCacheMutex);
		return mCachedSignatures.has(HalfSignature(transaction));
	}

	void Group::fillSignatureCacheOnStartup()
	{
		Poco::ScopedLock<Poco::FastMutex> _lock(mSignatureCacheMutex);
		int blockNr = mLastBlockNr;
		int transactionNr = mLastTransactionId;

		std::unique_ptr<model::gradido::GradidoBlock> transaction;
		Poco::DateTime border = Poco::DateTime() - Poco::Timespan(MAGIC_NUMBER_SIGNATURE_CACHE_MINUTES * 60, 0);

		do {
			if (transactionNr <= 0) break;
			if (blockNr <= 0) break;

			auto block = getBlock(blockNr);
			if (transactionNr < block->getBlockIndex()->getMinTransactionNr()) {
				blockNr--;
				continue;
			}
			// TODO: reverse order to read in transactions ascending to prevent jumping back in forth in file (seek trigger a new block read from mostly 8K)
			auto transactionEntry = block->getTransaction(transactionNr);
			transaction = std::make_unique<model::gradido::GradidoBlock>(transactionEntry->getSerializedTransaction());
			auto sigPairs = transaction->getGradidoTransaction()->getProto()->sig_map().sigpair();
			if (sigPairs.size()) {
				auto signature = DataTypeConverter::binToHex((const unsigned char*)sigPairs.Get(0).signature().data(), sigPairs.Get(0).signature().size());
				//printf("[Group::fillSignatureCacheOnStartup] add signature: %s\n", signature.data());
				HalfSignature transactionSign(sigPairs.Get(0).signature().data());
				mCachedSignatures.add(transactionSign, nullptr);
			}
			transactionNr--;
			//printf("compare received: %" PRId64 " with border : %" PRId64 "\n", transaction->getReceived().timestamp().raw(), border.timestamp().raw());
			
		} while (transaction && transaction->getReceived() > border);

	}

	void Group::calculateFinalBalance(Poco::SharedPtr<model::gradido::GradidoBlock> newGradidoBlock)
	{
		throw std::runtime_error("not implemented yet");
	}
}
