#include "Group.h"
#include "RemoteGroup.h"
#include "sodium.h"
#include "../ServerGlobals.h"

#include "../SystemExceptions.h"
#include "ControllerExceptions.h"

#include "../SingletonManager/GroupManager.h"
#include "../SingletonManager/LoggerManager.h"

#include "gradido_blockchain/lib/DataTypeConverter.h"
#include "gradido_blockchain/lib/Decay.h"
#include "gradido_blockchain/model/TransactionFactory.h"
#include "gradido_blockchain/model/protobufWrapper/TransactionValidationExceptions.h"
#include "gradido_blockchain/model/protobufWrapper/ProtobufExceptions.h"

#include "Block.h"

#include <inttypes.h>

#include "rapidjson/document.h"
#include "rapidjson/prettywriter.h"

#include "Poco/File.h"
#include "Poco/RegularExpression.h"
#include "Poco/Util/ServerApplication.h"

using namespace rapidjson;

namespace controller {

	Group::Group(std::string groupAlias, Poco::Path folderPath)
		: mIotaMessageListener(nullptr), mGroupAlias(groupAlias),
		mFolderPath(folderPath), mGroupState(Poco::Path(folderPath, ".state")), mDeferredTransfersCache(folderPath),
		mLastAddressIndex(0), mLastBlockNr(1), mLastTransactionId(0), mCachedBlocks(ServerGlobals::g_CacheTimeout * 1000),
		mCachedSignatureTransactionNrs(static_cast<Poco::Timestamp::TimeDiff>(MAGIC_NUMBER_SIGNATURE_CACHE_MINUTES * 1000 * 60)),
		mMessageIdTransactionNrCache(ServerGlobals::g_CacheTimeout * 1000),
		mCommunityServer(nullptr), mArchiveTransactionOrdering(nullptr), mExitCalled(false)
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
		mAddressIndex = new AddressIndex(folderPath, mLastAddressIndex, this);
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
			mCommunityServer = nullptr;
		}
		if (mArchiveTransactionOrdering) {
			delete mArchiveTransactionOrdering;
			mArchiveTransactionOrdering = nullptr;
		}
	}

	bool Group::init()
	{
		fillSignatureCacheOnStartup();
		mIotaMessageListener = new iota::MessageListener("GRADIDO." + mGroupAlias);
		return true;
	}

	void Group::setListeningCommunityServer(client::Base* client)
	{
		Poco::ScopedLock<Poco::Mutex> lock(mWorkingMutex);
		if (mCommunityServer) {
			delete mCommunityServer;
		}
		mCommunityServer = client;
	}

	void Group::resetAllIndices()
	{
		LoggerManager::getInstance()->mErrorLogging.warning("[Group::resetAllIndices called] something went wrong with the index files");
		Poco::ScopedLock _lock(mWorkingMutex);
		// clear indices from memory
		mCachedBlocks.clear();
		mAddressIndex.reset();
		updateLastAddressIndex(0);

		// clear indices from mass storage
		// clear address indices
		Poco::Path addressIndicesFolder(mFolderPath);
		addressIndicesFolder.pushDirectory("pubkeys");
		Poco::File pubkeyFolder(addressIndicesFolder);
		pubkeyFolder.remove(true);

		// clear block indices
		Poco::File groupFolder(mFolderPath);
		Poco::Path::StringVec files;
		groupFolder.list(files);
		// *.index
		Poco::RegularExpression isIndexFile(".*\.index$");
		std::for_each(files.begin(), files.end(), [&](const std::string& file) {
			if (isIndexFile.match(file)) {
				Poco::File fileForDeletion(mFolderPath.toString() + file);
				fileForDeletion.remove();
			}
		});
		mAddressIndex = new AddressIndex(mFolderPath, mLastAddressIndex, this);
	}

	ArchiveTransactionsOrdering* Group::getArchiveTransactionsOrderer()
	{
		if (!mArchiveTransactionOrdering) {
			mArchiveTransactionOrdering = new ArchiveTransactionsOrdering(this);
		}
		return mArchiveTransactionOrdering;
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
		if (lastTransaction) {
			if (newGradidoBlock->getReceived() < lastTransaction->getReceived()) {
				throw BlockchainOrderException("previous transaction is younger");
			}
			if (newGradidoBlock->getReceived() == lastTransaction->getReceived()) {
				if (!newGradidoBlock->getGradidoTransaction()->getSerializedConst()->compare(*lastTransaction->getGradidoTransaction()->getSerializedConst())) {
					throw GradidoBlockchainTransactionAlreadyExistException("[Group::addTransaction] skip because already exist 2");
				}
			}
		}


		try {
			// calculate final balance
			newGradidoBlock->calculateFinalGDD(this);
		}
		catch (Poco::NullPointerException& ex) {
			LoggerManager::getInstance()->mErrorLogging.error("poco null pointer exception by calling GradidoBlock::calculateFinalGDD");
			throw;
		}

		// calculate tx hash
		MemoryBin* txHash = nullptr;
		if (lastTransaction) {
			txHash = newGradidoBlock->calculateTxHash(lastTransaction.get());
		}
		else {
			txHash = newGradidoBlock->calculateTxHash(nullptr);
		}
		newGradidoBlock->setTxHash(txHash);
		MemoryManager::getInstance()->releaseMemory(txHash);

		// intern validation
		model::gradido::TransactionValidationLevel level = (model::gradido::TransactionValidationLevel)(
			model::gradido::TRANSACTION_VALIDATION_SINGLE |
			model::gradido::TRANSACTION_VALIDATION_SINGLE_PREVIOUS |
			model::gradido::TRANSACTION_VALIDATION_DATE_RANGE |
			model::gradido::TRANSACTION_VALIDATION_CONNECTED_GROUP
			);
		auto transactionBody = newGradidoBlock->getGradidoTransaction()->getTransactionBody();
		if (!transactionBody->isLocal()) {
			level = (model::gradido::TransactionValidationLevel)(level | model::gradido::TRANSACTION_VALIDATION_PAIRED);
		}
		//printf("validate with level: %d\n", level);
		auto otherGroup = transactionBody->getOtherGroup();
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
		try {
			newGradidoBlock->validate(level, this, otherBlockchain);
		}
		catch (Poco::NullPointerException& ex) {
			LoggerManager::getInstance()->mErrorLogging.error("poco null pointer exception by calling validate with level: %u", (unsigned)level);
			throw;
		}

		Poco::ScopedLock<Poco::Mutex> lock(mWorkingMutex);
		auto block = getCurrentBlock();
		if (block.isNull()) {
			throw BlockNotLoadedException("don't get a valid current block", mGroupAlias, mLastBlockNr);
		}
		//TransactionEntry(uint64_t _transactionNr, std::string _serializedTransaction, Poco::DateTime received, uint16_t addressIndexCount = 2);
		Poco::SharedPtr<model::NodeTransactionEntry> transactionEntry = new model::NodeTransactionEntry(newGradidoBlock, mAddressIndex);
		bool result = false;
		try {
			result = block->pushTransaction(transactionEntry);
		}
		catch (Poco::NullPointerException& ex) {
			LoggerManager::getInstance()->mErrorLogging.error("poco null pointer exception by calling Block::pushTransaction");
			throw;
		}
		if (result) {
			mLastTransaction = newGradidoBlock;
			updateLastTransactionId(newGradidoBlock->getID());
			// TODO: move call to better place
			updateLastAddressIndex(mAddressIndex->getLastIndex());
			addSignatureToCache(newGradidoBlock);
			if (messageId) {
				mMessageIdTransactionNrCacheMutex.lock();
				iota::MessageId mid; mid.fromMemoryBin(messageId);
				mMessageIdTransactionNrCache.add(mid, mLastTransaction->getID());
				mMessageIdTransactionNrCacheMutex.unlock();
			}

			//std::clog << "add transaction: " << mLastTransaction->getID() << ", memo: " << newTransaction->getTransactionBody()->getMemo() << std::endl;
			/*printf("[%s] nr: %d, group: %s, messageId: %s, received: %d, transaction: %s",
				__FUNCTION__, mLastTransaction->getID(), mGroupAlias.data(), mLastTransaction->getMessageIdHex().data(),
				mLastTransaction->getReceived(), mLastTransaction->getGradidoTransaction()->toJson().data()
			);*/
			// say community server that a new transaction awaits him
			if (mCommunityServer) {
				try {
					mCommunityServer->notificateNewTransaction(mLastTransaction);
				}
				catch (Poco::NullPointerException& ex) {
					LoggerManager::getInstance()->mErrorLogging.error("poco null pointer exception by calling notificateNewTransaction");
					throw;
				}
			}
		}
		return result;
	}

	Poco::SharedPtr<model::TransactionEntry> Group::findLastTransactionForAddress(const std::string& address, const std::string& coinGroupId/* = ""*/)
	{
		Poco::ScopedLock<Poco::Mutex> lock(mWorkingMutex);
		auto index = mAddressIndex->getIndexForAddress(address);

		// if we don't know the index, we haven't heard from this address at all
		if (!index) {
			return nullptr;
		}

		auto blockNr = mLastBlockNr;
		uint64_t transactionNr = 0;

		do {
			auto block = getBlock(blockNr);
			auto blockIndex = block->getBlockIndex();
			transactionNr = blockIndex->findLastTransactionForAddress(index, coinGroupId);
			if (transactionNr) {
				return block->getTransaction(transactionNr);
			}
			blockNr--;
		} while (blockNr > 0);

		return nullptr;
	}

	//std::vector<Poco::AutoPtr<model::GradidoBlock>> Group::findTransactions(uint64_t fromTransactionId)
	std::vector<Poco::SharedPtr<model::TransactionEntry>> Group::findTransactionsFromXToLast(uint64_t fromTransactionId)
	{
		Poco::ScopedLock<Poco::Mutex> lock(mWorkingMutex);
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

	std::vector<Poco::SharedPtr<model::TransactionEntry>> Group::findTransactions(const std::string& address)
	{
		Poco::ScopedLock<Poco::Mutex> lock(mWorkingMutex);
		std::vector<Poco::SharedPtr<model::TransactionEntry>> transactions;

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

	std::vector<uint64_t> Group::findTransactionIds(const std::string& address)
	{
		Poco::ScopedLock<Poco::Mutex> lock(mWorkingMutex);
		std::vector<uint64_t> transactions;

		auto index = mAddressIndex->getIndexForAddress(address);
		if (!index) { return transactions; }

		int blockCursor = mLastBlockNr;
		while (blockCursor > 0) {
			auto block = getBlock(blockCursor);
			auto blockIndex = block->getBlockIndex();
			auto transactionNrs = blockIndex->findTransactionsForAddress(index);
			if (transactionNrs.size()) {
				transactions.insert(
					transactions.end(),
					std::make_move_iterator(transactionNrs.begin()),
					std::make_move_iterator(transactionNrs.end())
				);
			}
			blockCursor--;
		}

		return transactions;
	}

	std::vector<Poco::SharedPtr<model::TransactionEntry>> Group::findTransactions(const std::string& address, int month, int year)
	{
		Poco::ScopedLock<Poco::Mutex> lock(mWorkingMutex);
		std::vector<Poco::SharedPtr<model::TransactionEntry>>  transactions;

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

	Poco::SharedPtr<model::gradido::GradidoBlock> Group::getLastTransaction(std::function<bool(const model::gradido::GradidoBlock*)> filter/* = nullptr*/)
	{
		Poco::ScopedLock<Poco::Mutex> lock(mWorkingMutex);
		if (filter) {
			int blockCursor = mLastBlockNr;
			while (blockCursor >= 0)
			{
				auto block = getBlock(blockCursor);
				auto blockIndex = block->getBlockIndex();
				// TODO: Check if search backwards make more sense
				for (int i = blockIndex->getMinTransactionNr(); i <= blockIndex->getMaxTransactionNr(); i++) {
					if (!i) break;
					auto transaction = block->getTransaction(i);
					Poco::SharedPtr<model::gradido::GradidoBlock> gradidoBlock(new model::gradido::GradidoBlock(transaction->getSerializedTransaction()));
					if (filter(gradidoBlock.get())) {
						return gradidoBlock;
					}
				}
				blockCursor--;
			}
			return nullptr;
		}
		if (mLastTransaction) {
			return mLastTransaction;
		}
		Poco::ScopedLock _lock(mWorkingMutex);
		auto block = getBlock(mLastBlockNr);
		auto blockIndex = block->getBlockIndex();
		auto lastTransactionId = mLastTransactionId;
		if (!lastTransactionId || lastTransactionId > blockIndex->getMaxTransactionNr()) {
			lastTransactionId = blockIndex->getMaxTransactionNr();
		}
		if (!lastTransactionId) {
			return nullptr;
		}
		// throw an exception if transaction wasn't found
		auto transaction = block->getTransaction(lastTransactionId);
		if (!transaction.isNull()) {
			updateLastTransactionId(lastTransactionId);
			mLastTransaction = new model::gradido::GradidoBlock(transaction->getSerializedTransaction());
			return mLastTransaction;
		}
		return nullptr;
	}

	mpfr_ptr Group::calculateAddressBalance(const std::string& address, const std::string& coinGroupId, Poco::DateTime date)
	{
		Poco::ScopedLock<Poco::Mutex> lock(mWorkingMutex);
		auto mm = MemoryManager::getInstance();
		auto gdd = mm->getMathMemory();
		auto addressIndex = getAddressIndex()->getIndexForAddress(address);
		// if return zero, no transaction for this address exist on this blockchain
		if (!addressIndex) return gdd;

		auto blockNr = mLastBlockNr;
		auto temp = MathMemory::create();

		std::unique_ptr<model::gradido::GradidoBlock> lastGradidoBlockWithFinalBalance;
		std::list<std::pair<mpfr_ptr, Poco::DateTime>> receiveTransfers;

		do
		{
			auto block = getBlock(blockNr);
			if (block.isNull()) {
				throw BlockNotLoadedException("[Group::calculateAddressBalance] block is null", mGroupAlias, blockNr);
			}
			auto blockIndex = block->getBlockIndex();
			if (blockIndex.isNull()) {
				throw BlockIndexException("[Group::calculateAddressBalance] block index is null");
			}
			auto transactionNrs = blockIndex->findTransactionsForAddress(addressIndex, coinGroupId);

			std::sort(transactionNrs.begin(), transactionNrs.end());
			// begin on last transaction
			for (auto it = transactionNrs.rbegin(); it != transactionNrs.rend(); ++it)
			{
				auto transactionEntry = block->getTransaction(*it);
				if (transactionEntry.isNull()) {
					throw GradidoBlockchainTransactionNotFoundException("[Group::calculateAddressBalance] transactionEntry is null");
				}
				auto gradidoBlock = std::make_unique<model::gradido::GradidoBlock>(transactionEntry->getSerializedTransaction());
				if (gradidoBlock->getReceivedAsTimestamp() > date.timestamp()) {
					continue;
				}
				auto transactionBody = gradidoBlock->getGradidoTransaction()->getTransactionBody();
				if (transactionBody->isTransfer() || transactionBody->isDeferredTransfer()) {
					auto transfer = transactionBody->getTransferTransaction();
					if (transfer->getSenderPublicKeyString() == address) {
						lastGradidoBlockWithFinalBalance = std::move(gradidoBlock);
						break;
					}
					else {
						auto receiveAmount = mm->getMathMemory();
						if (mpfr_set_str(receiveAmount, transfer->getAmount().data(), 10, gDefaultRound)) {
							throw model::gradido::TransactionValidationInvalidInputException("amount cannot be parsed to a number", "amount", "string");
						}
						if (transactionBody->isDeferredTransfer()) {
							auto deferredTransfer = transactionBody->getDeferredTransfer();
							if (deferredTransfer->getTimeoutAsPocoDateTime() <= date) {
								continue;
							}
							// subtract decay from time: date -> timeout
							// later in code the decay for: received -> date will be subtracted automatic
							// TODO: check if result is like expected
							calculateDecayFactorForDuration(
								temp->getData(), 
								gDecayFactorGregorianCalender, 
								date, deferredTransfer->getTimeoutAsPocoDateTime()
							);
							calculateDecayFast(temp->getData(), receiveAmount);
						}

						receiveTransfers.push_front({ receiveAmount , gradidoBlock->getReceivedAsTimestamp() });
						if (transactionBody->isDeferredTransfer()) {
							break;
						}
					}
				}
				else if (transactionBody->isCreation() || transactionBody->isRegisterAddress()) {
					lastGradidoBlockWithFinalBalance = std::move(gradidoBlock);
					break;
				}
			}

			blockNr--;
		} while (blockNr > 0 && !lastGradidoBlockWithFinalBalance);

		// calculate decay
		Poco::DateTime lastDate;
		if (lastGradidoBlockWithFinalBalance) {
			lastDate = lastGradidoBlockWithFinalBalance->getReceivedAsTimestamp();
			if (mpfr_set_str(gdd, lastGradidoBlockWithFinalBalance->getFinalBalance().data(), 10, gDefaultRound)) {
				throw model::gradido::TransactionValidationInvalidInputException("finalBalance cannot be parsed to a number", "finalBalance", "string");
			}
		}
		else if(receiveTransfers.size()) {
			// if no lastGradidoBlockWithFinalBalance was found because sender is deferred transfer or not registered
			// use first received transfer as starting point
			auto firstReceived = receiveTransfers.front();
			mpfr_add(gdd, gdd, firstReceived.first, gDefaultRound);
			lastDate = firstReceived.second;
		}

		// check for time outed deferred transfers which will be automatic booked back
		auto deferredTransfers = getTimeoutedDeferredTransferReturnedAmounts(addressIndex, lastDate, date);
		if (deferredTransfers.size()) {
			auto deferredTransfersIt = deferredTransfers.begin();
			for (auto it = receiveTransfers.begin(); it != receiveTransfers.end(); it++) {
				while (it->second > deferredTransfersIt->second && deferredTransfersIt != deferredTransfers.end()) {
					receiveTransfers.insert(it, *deferredTransfersIt);
					deferredTransfersIt++;

				}
				if (deferredTransfersIt == deferredTransfers.end()) {
					break;
				}
			}
		}
		for (auto it = receiveTransfers.begin(); it != receiveTransfers.end(); it++) {
			assert(it->second >= lastDate);
			if (it->second > lastDate) {
				calculateDecayFactorForDuration(
					temp->getData(), 
					gDecayFactorGregorianCalender, 
					lastDate, it->second
				);
				calculateDecayFast(temp->getData(), gdd);
				lastDate = it->second;
			}
			mpfr_add(gdd, gdd, it->first, gDefaultRound);
			mm->releaseMathMemory(it->first);
		}
		// cmp return 0 if gdd == 0
		if (!mpfr_cmp_si(gdd, 0)) {
			return gdd;
		}
		assert(date >= lastDate);
		if (Poco::Timespan(date - lastDate).totalSeconds()) {
			calculateDecayFactorForDuration(
				temp->getData(), 
				gDecayFactorGregorianCalender, 
				lastDate, date
			);
			calculateDecayFast(temp->getData(), gdd);
		}

		return gdd;
	}

	proto::gradido::RegisterAddress_AddressType Group::getAddressType(const std::string& address)
	{
		auto addressIndex = getAddressIndex()->getIndexForAddress(address);
		// if return zero, no transaction for this address exist on this blockchain
		if (!addressIndex) return proto::gradido::RegisterAddress_AddressType_NONE;

		int blockNr = 1;
		while (blockNr <= mLastBlockNr) {
			auto block = getBlock(blockNr);
			auto transactionNr = block->getBlockIndex()->findFirstTransactionForAddress(addressIndex);
			if (transactionNr) {
				auto transactionEntry = block->getTransaction(transactionNr);
				auto gradidoBlock = std::make_unique<model::gradido::GradidoBlock>(transactionEntry->getSerializedTransaction());
				auto transactionBody = gradidoBlock->getGradidoTransaction()->getTransactionBody();
				if (!transactionBody->isRegisterAddress()) {
					throw WrongTransactionTypeException(
						"wrong transaction type for first transaction",
						transactionBody->getTransactionType(),
						std::move(DataTypeConverter::binToHex(address))
					);
				}
				return transactionBody->getRegisterAddress()->getAddressType();
			}
			blockNr++;
		}
		return proto::gradido::RegisterAddress_AddressType_NONE;
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
		return nullptr;
	}

	std::vector<Poco::SharedPtr<model::TransactionEntry>> Group::searchTransactions(
		uint64_t startTransactionNr/* = 0*/,
		std::function<FilterResult(model::TransactionEntry*)> filter/* = nullptr*/,
		SearchDirection order /*= SearchDirection::ASC*/
	)
	{
		Poco::ScopedLock<Poco::Mutex> lock(mWorkingMutex);
		std::vector<Poco::SharedPtr<model::TransactionEntry>> result;
		auto lastBlock = getBlock(mLastBlockNr);
		if (!filter) {
			result.reserve(lastBlock->getBlockIndex()->getMaxTransactionNr());
		}
		if (SearchDirection::ASC == order) {
			int blockCursor = 1;
			while (blockCursor <= mLastBlockNr)
			{
				auto block = getBlock(blockCursor);
				auto blockIndex = block->getBlockIndex();
				if (blockIndex->getMaxTransactionNr() < startTransactionNr) continue;
				for (int i = blockIndex->getMinTransactionNr(); i <= blockIndex->getMaxTransactionNr(); ++i) {
					if (!i || i < startTransactionNr) break;
					auto transaction = block->getTransaction(i);
					FilterResult filterResult = FilterResult::USE;
					if (filter) {
						filterResult = filter(transaction);
					}
					if (filterResult == (filterResult & FilterResult::USE)) {
						result.push_back(block->getTransaction(i));
					}
					if (filterResult == (filterResult & FilterResult::STOP)) {
						return std::move(result);
					}
				}
				blockCursor++;
			}
		}
		else if (SearchDirection::DESC == order) {
			int blockCursor = mLastBlockNr;
			while (blockCursor > 0)
			{
				auto block = getBlock(blockCursor);
				auto blockIndex = block->getBlockIndex();
				for (int i = blockIndex->getMaxTransactionNr(); i >= blockIndex->getMinTransactionNr(); --i) {
					if (!i) break;
					auto transaction = block->getTransaction(i);
					FilterResult filterResult = FilterResult::USE;
					if (filter) {
						filterResult = filter(transaction);
					}
					if (filterResult == (filterResult & FilterResult::USE)) {
						result.push_back(block->getTransaction(i));
					}
					if (filterResult == (filterResult & FilterResult::STOP)) {
						return std::move(result);
					}
				}
				blockCursor--;
			}
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
		if (!block->init(mAddressIndex)) {
			// TODO: request block from neighbor gradido node or check if all transactions which should be in this block are still present on iota
			LoggerManager::getInstance()->mErrorLogging.critical(
				"[controller::Group::getBlock] block init return false, maybe block file is corrupt, please delete block file and index and all which came after and try again"
			);
			Poco::Util::ServerApplication::terminate();
		}
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
		if (mCachedSignatureTransactionNrs.has(transactionSign)) {
			// additional check if transaction really exactly the same, because hash collisions are possible, albeit with a very low probability
			auto transactionNr = mCachedSignatureTransactionNrs.get(transactionSign);
			auto cachedGradidoBlock = std::make_unique<model::gradido::GradidoBlock>(getTransactionForId(*transactionNr)->getSerializedTransaction());
			// compare return 0 if strings are the same
			if (!cachedGradidoBlock->getGradidoTransaction()->getSerializedConst()->compare(*transaction->getSerializedConst())) {
				return true;
			}
			else {
				return false;
			}
		}
		return false;
	}

	void Group::addSignatureToCache(Poco::SharedPtr<model::gradido::GradidoBlock> gradidoBlock)
	{
		Poco::ScopedLock<Poco::FastMutex> _lock(mSignatureCacheMutex);
		mCachedSignatureTransactionNrs.add(HalfSignature(gradidoBlock->getGradidoTransaction()), gradidoBlock->getID());
	}


	void Group::fillSignatureCacheOnStartup()
	{
		Poco::ScopedLock<Poco::FastMutex> _lock(mSignatureCacheMutex);
		int blockNr = mLastBlockNr;
		int transactionNr = mLastTransactionId;

		std::unique_ptr<model::gradido::GradidoBlock> transaction;
		Poco::Timestamp border = Poco::Timestamp() - Poco::Timespan(MAGIC_NUMBER_SIGNATURE_CACHE_MINUTES * 60, 0);

		do {
			if (transactionNr <= 0) break;
			if (blockNr <= 0) break;

			auto block = getBlock(blockNr);
			if (transactionNr < block->getBlockIndex()->getMinTransactionNr()) {
				blockNr--;
				continue;
			}
			// TODO: reverse order to read in transactions ascending to prevent jumping back in forth in file (seek trigger a new block read from mostly 8K)
			try {
				auto transactionEntry = block->getTransaction(transactionNr);
				auto gradidoBlock = std::make_unique<model::gradido::GradidoBlock>(transactionEntry->getSerializedTransaction());
				mCachedSignatureTransactionNrs.add(HalfSignature(gradidoBlock->getGradidoTransaction()), gradidoBlock->getID());
			}
			catch (GradidoBlockchainTransactionNotFoundException& ex) {
				LoggerManager::getInstance()->mErrorLogging.warning("transaction: %d not found, reset state: %s", transactionNr, ex.getFullString());
				if (transactionNr == 1) {
					updateLastTransactionId(0);
					break;
				}
			}

			transactionNr--;
			//printf("compare received: %" PRId64 " with border : %" PRId64 "\n", transaction->getReceived().timestamp().raw(), border.timestamp().raw());

		} while (transaction && transaction->getReceivedAsTimestamp() > border);

	}

	std::vector<std::pair<mpfr_ptr, Poco::DateTime>>  Group::getTimeoutedDeferredTransferReturnedAmounts(uint32_t addressIndex, Poco::DateTime beginDate, Poco::DateTime endDate)
	{
		auto mm = MemoryManager::getInstance();
		auto deferredTransfers = mDeferredTransfersCache.getTransactionNrsForAddressIndex(addressIndex);
		std::vector<std::pair<mpfr_ptr, Poco::DateTime>> timeoutedDeferredTransfers;
		for (auto it = deferredTransfers.begin(); it != deferredTransfers.end(); it++) {
			auto transactionEntry = getTransactionForId(*it);
			auto gradidoBlock = std::make_unique<model::gradido::GradidoBlock>(transactionEntry->getSerializedTransaction());
			auto transactionBody = gradidoBlock->getGradidoTransaction()->getTransactionBody();
			// little error correction
			if (!transactionBody->isDeferredTransfer()) {
				mDeferredTransfersCache.removeTransactionNrForAddressIndex(addressIndex, *it);
				continue;
			}
			auto deferredTransfer = gradidoBlock->getGradidoTransaction()->getTransactionBody()->getDeferredTransfer();
			auto timeout = deferredTransfer->getTimeoutAsPocoDateTime();
			if (timeout < beginDate || timeout > endDate) {
				continue;
			}

			// check if someone has already used it
			auto balance = calculateAddressBalance(deferredTransfer->getRecipientPublicKeyString(), deferredTransfer->getCoinGroupId(), timeout);
			if (mpfr_cmp_si(balance, 0) > 0) {
				timeoutedDeferredTransfers.push_back({ balance, timeout });
			}
			else {
				mDeferredTransfersCache.removeTransactionNrForAddressIndex(addressIndex, gradidoBlock->getID());
				mm->releaseMathMemory(balance);
			}
		}
		return timeoutedDeferredTransfers;
	}

}
