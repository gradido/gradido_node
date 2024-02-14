#ifndef __GRADIDO_NODE_CONTROLLER_GROUP_H
#define __GRADIDO_NODE_CONTROLLER_GROUP_H

#include <chrono>

#include "ControllerBase.h"
#include "MemoryAddressIndex.h"
//#include "Block.h"
#include "BlockIndex.h"
#include "TaskObserver.h"
#include "DeferredTransfer.h"
#include "ArchiveTransactionsOrdering.h"

#include "gradido_blockchain/model/protobufWrapper/ConfirmedTransaction.h"
#include "gradido_blockchain/model/protobufWrapper/GradidoTransaction.h"
#include "../model/files/State.h"

#include "gradido_blockchain/http/JsonRequest.h"
#include "../iota/MessageListener.h"

#include "../client/Base.h"

#include "Poco/Path.h"
#include "Poco/AutoPtr.h"
#include "Poco/URI.h"
#include "Poco/AccessExpireCache.h"
#include "Poco/ExpireCache.h"

#include "gradido_blockchain/model/IGradidoBlockchain.h"

#include <algorithm> 

// MAGIC NUMBER: how long signatures should be cached to prevent processing transactions more than once
// should be at least MAGIC_NUMBER_MAX_TIMESPAN_BETWEEN_CREATING_AND_RECEIVING_TRANSACTION_IN_MINUTES minutes, 
// this is the maximal allowed timespan between created and received date in transactions
#define MAGIC_NUMBER_SIGNATURE_CACHE_MINUTES std::chrono::minutes(std::max(10, MAGIC_NUMBER_MAX_TIMESPAN_BETWEEN_CREATING_AND_RECEIVING_TRANSACTION_IN_MINUTES))

namespace controller {

	class Block;
	/*!
	* @author Dario Rekowski
	* @date 2020-02-06
	* @brief Main entry point for adding and searching for transactions
	* TODO: validate transaction on startup
	*/

	class Group : public ControllerBase, public model::IGradidoBlockchain
	{
	public:
		//! \brief Load group states via model::files::GroupState.
		Group(std::string alias, Poco::Path folderPath);
		virtual ~Group();

		// initialize, fill cache
		bool init();
		// clean up group, stopp all running processes
		void exit();

		
		//! \brief add transaction from iota into blockchain, important! must be called in correct order
		virtual bool addTransaction(std::unique_ptr<model::gradido::GradidoTransaction> newTransaction, const MemoryBin* messageId, uint64_t iotaMilestoneTimestamp);

		//! \brief Find last transaction of address account in memory or block chain.
		//! \param address Address = user account public key.
		Poco::SharedPtr<model::TransactionEntry> findLastTransactionForAddress(const std::string& address, const std::string& coinGroupId = "");

		//! \brief return last transaction which was added to this blockchain
		Poco::SharedPtr<model::gradido::ConfirmedTransaction> getLastTransaction(std::function<bool(const model::gradido::ConfirmedTransaction*)> filter = nullptr);

		//! \brief get last transaction for this user for this coin color with a final balance
		// TODO: make UML diagram for function
		mpfr_ptr calculateAddressBalance(const std::string& address, const std::string& groupId, Poco::DateTime date, uint64_t ownTransactionNr);

		proto::gradido::RegisterAddress_AddressType getAddressType(const std::string& address);

		Poco::SharedPtr<model::TransactionEntry> getTransactionForId(uint64_t transactionId);

		//! \brief find transaction by messageId, especially used for validate cross group transactions
		Poco::SharedPtr<model::TransactionEntry> findByMessageId(const MemoryBin* messageId, bool cachedOnly = true);

		//! \brief return vector with transaction entries from xTransactionId until last known transaction
		std::vector<Poco::SharedPtr<model::TransactionEntry>> findTransactionsFromXToLast(uint64_t xTransactionId);
		//! \brief Find every transaction belonging to address account in memory or block chain, expensive.
		//!
		//! Use with care, can need some time and return huge amount of data.
		//! \param address Address = user account public key.
		std::vector<Poco::SharedPtr<model::TransactionEntry>> findTransactions(const std::string& address);

		//! \brief find transaction belonging to address account but with pagination
		std::vector<Poco::SharedPtr<model::TransactionEntry>> findTransactions(const std::string& address, uint32_t maxResultCount, uint64_t startTransactionNr = 1);

		/*! \brief Find every transaction belonging to address account in memory or block chain, expensive.
			
			Faster than findTransactions because it need only reading block index
			\param address Address = user account public key.
		*/
		std::vector<uint64_t> findTransactionIds(const std::string& address);
		//! \brief Find transactions of account in memory or block chain from a specific month.
		//! \param address User account public key.
		std::vector<Poco::SharedPtr<model::TransactionEntry>> findTransactions(const std::string& address, int month, int year);

		//! \brief go through all transaction and return transactions
		std::vector<Poco::SharedPtr<model::TransactionEntry>> searchTransactions(
			uint64_t startTransactionNr = 0,
			std::function<FilterResult(model::TransactionEntry*)> filter = nullptr,
			SearchDirection order = SearchDirection::ASC
		);

		inline Poco::SharedPtr<AddressIndex> getAddressIndex() { return mAddressIndex; }

		//! \brief group alias or group id, the single identifier for group blockchain and group coins
		inline const std::string& getGroupAlias() const { return mGroupAlias; }
		//! \brief group id is the same as group alias
		inline const std::string& getCommunityId() const { return mGroupAlias; }

		bool isTransactionAlreadyExist(const model::gradido::GradidoTransaction* transaction);
		void setListeningCommunityServer(client::Base* client);
		inline client::Base* getListeningCommunityServer() const { return mCommunityServer; }

		// expensive, remove all address and block indices, they must be build from scratch
		void resetAllIndices();

		ArchiveTransactionsOrdering* getArchiveTransactionsOrderer();

	protected:
		void updateLastAddressIndex(int lastAddressIndex);
		void updateLastBlockNr(int lastBlockNr);
		void updateLastTransactionId(int lastTransactionId);

		
		void addSignatureToCache(Poco::SharedPtr<model::gradido::ConfirmedTransaction> gradidoBlock);
		//! read blocks starting by latest until block is older than MILESTONES_BOOTSTRAP_COUNT * 1.5 minutes | io read expensive
		//! put all signatures from young enough blocks into signature cache
		virtual void fillSignatureCacheOnStartup();

		TaskObserver mTaskObserver;
		iota::MessageListener* mIotaMessageListener;
		std::string mGroupAlias;
		Poco::Path mFolderPath;
		Poco::SharedPtr<AddressIndex> mAddressIndex;
		model::files::State mGroupState;
		DeferredTransfer    mDeferredTransfersCache;

		std::vector<std::pair<mpfr_ptr, Poco::DateTime>> getTimeoutedDeferredTransferReturnedAmounts(uint32_t addressIndex, Poco::DateTime beginDate, Poco::DateTime endDate);

		Poco::SharedPtr<model::gradido::ConfirmedTransaction> mLastTransaction;
		int mLastAddressIndex;
		int mLastBlockNr;
		int mLastTransactionId;
		//std::list<BlockEntry> mBlocks;
		Poco::AccessExpireCache<Poco::UInt32, Block> mCachedBlocks;

		//! \brief get current block to write more transactions in it
		Poco::SharedPtr<Block> getCurrentBlock();
		Poco::SharedPtr<Block> getBlock(Poco::UInt32 blockNr);
		Poco::SharedPtr<Block> getBlockContainingTransaction(uint64_t transactionId);

		// for preventing double transactions
		// keep first 32 Byte of first signature of each transaction from last MAGIC_NUMBER_SIGNATURE_CACHE_MINUTES minutes
		struct HalfSignature
		{
			HalfSignature(const char* signature) {
				memcpy(&sign, signature, 32);
			}
			HalfSignature(const model::gradido::GradidoTransaction* transaction) {
				auto sigPairs = transaction->getSignaturesfromSignatureMap(true);
				if (sigPairs.size() == 0) {
					throw std::runtime_error("[Group::addSignatureToCache] empty signatures");
				}

				memcpy(&sign, sigPairs[0]->data(), 32);
				auto mm = MemoryManager::getInstance();
				mm->releaseMemory(sigPairs[0]);
			}
			bool operator<(const HalfSignature& ob) const {
				return
					sign[0] < ob.sign[0] ||
					(sign[0] == ob.sign[0] && sign[1] < ob.sign[1]) || (
						sign[0] == ob.sign[0] &&
						sign[1] == ob.sign[1] &&
						sign[2] < ob.sign[2]
						) || (
							sign[0] == ob.sign[0] &&
							sign[1] == ob.sign[1] &&
							sign[2] == ob.sign[2] &&
							sign[3] < ob.sign[3]
							);
			}
			//bool comp(a, b)
			int64_t sign[4];
		};
		Poco::ExpireCache<HalfSignature, uint64_t> mCachedSignatureTransactionNrs;
		mutable Poco::FastMutex mSignatureCacheMutex;

		Poco::ExpireCache<iota::MessageId, uint64_t> mMessageIdTransactionNrCache;
		mutable Poco::FastMutex mMessageIdTransactionNrCacheMutex;

		// Community Server listening on new blocks for his group
		client::Base* mCommunityServer;
		ArchiveTransactionsOrdering* mArchiveTransactionOrdering;

		bool mExitCalled;
	};
}

#endif //__GRADIDO_NODE_CONTROLLER_GROUP_H
