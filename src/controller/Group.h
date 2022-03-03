#ifndef __GRADIDO_NODE_CONTROLLER_GROUP_H
#define __GRADIDO_NODE_CONTROLLER_GROUP_H

#include "ControllerBase.h"
#include "AddressIndex.h"
//#include "Block.h"
#include "BlockIndex.h"
#include "TaskObserver.h"

#include "gradido_blockchain/model/protobufWrapper/GradidoBlock.h"
#include "gradido_blockchain/model/protobufWrapper/GradidoTransaction.h"
#include "../model/files/State.h"

#include "gradido_blockchain/http/JsonRequest.h"
#include "../iota/MessageListener.h"

#include "Poco/Path.h"
#include "Poco/AutoPtr.h"
#include "Poco/URI.h"
#include "Poco/AccessExpireCache.h"
#include "Poco/ExpireCache.h"

#include "gradido_blockchain/model/IGradidoBlockchain.h"

// MAGIC NUMBER: how long signatures should be cached to prevent processing transactions more than once
#define MAGIC_NUMBER_SIGNATURE_CACHE_MINUTES 10

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
		Group(std::string alias, Poco::Path folderPath, uint32_t coinColor);
		virtual ~Group();

		// initialize, fill cache
		bool init();
		// clean up group, stopp all running processes
		void exit();

		
		//! \brief add transaction from iota into blockchain, important! must be called in correct order
		virtual bool addTransaction(std::unique_ptr<model::gradido::GradidoTransaction> newTransaction, const MemoryBin* messageId, uint64_t iotaMilestoneTimestamp);

		//! \brief Find last transaction of address account in memory or block chain.
		//! \param address Address = user account public key.
		Poco::AutoPtr<model::gradido::GradidoBlock> findLastTransaction(const std::string& address);

		//! \brief return last transaction which was added to this blockchain
		Poco::SharedPtr<model::gradido::GradidoBlock> getLastTransaction();

		Poco::SharedPtr<model::TransactionEntry> getTransactionForId(uint64_t transactionId);

		//! \brief find transaction by messageId, especially used for validate cross group transactions
		Poco::SharedPtr<model::TransactionEntry> findByMessageId(const MemoryBin* messageId, bool cachedOnly = true);

		//! \brief return vector with transaction entries from xTransactionId until last known transaction
		std::vector<Poco::SharedPtr<model::TransactionEntry>> findTransactionsFromXToLast(uint64_t xTransactionId);
		//! \brief Find every transaction belonging to address account in memory or block chain, expensive.
		//!
		//! Use with care, can need some time and return huge amount of data.
		//! \param address Address = user account public key.
		std::vector<Poco::SharedPtr<model::NodeTransactionEntry>> findTransactions(const std::string& address);
		//! \brief Find transactions of account in memory or block chain from a specific month.
		//! \param address User account public key.
		std::vector<Poco::SharedPtr<model::NodeTransactionEntry>> findTransactions(const std::string& address, int month, int year);

		//! \brief go through all transaction and return transactions
		std::vector<Poco::SharedPtr<model::TransactionEntry>> getAllTransactions(std::function<bool(model::TransactionEntry*)> filter = nullptr);

		//! \brief Search for creation transactions from specific month and add balances together.
		//! \param address User account public key.
		void calculateCreationSum(const std::string& address, int month, int year, Poco::DateTime received, mpfr_ptr sum);

		inline Poco::SharedPtr<AddressIndex> getAddressIndex() { return mAddressIndex; }

		inline const std::string& getGroupAlias() { return mGroupAlias; }
		inline uint32_t getGroupDefaultCoinColor() const { return mCoinColor; }

		bool isSignatureInCache(const model::gradido::GradidoTransaction* transaction);

		void setListeningCommunityServer(Poco::URI uri);


	protected:
		void updateLastAddressIndex(int lastAddressIndex);
		void updateLastBlockNr(int lastBlockNr);
		void updateLastTransactionId(int lastTransactionId);

		bool isTransactionAlreadyExist(const model::gradido::GradidoTransaction* transaction);
		void addSignatureToCache(Poco::SharedPtr<model::gradido::GradidoBlock> gradidoBlock);
		//! read blocks starting by latest until block is older than MILESTONES_BOOTSTRAP_COUNT * 1.5 minutes | io read expensive
		//! put all signatures from young enough blocks into signature cache
		virtual void fillSignatureCacheOnStartup();

		void calculateFinalBalance(Poco::SharedPtr<model::gradido::GradidoBlock> newGradidoBlock);

		TaskObserver mTaskObserver;
		iota::MessageListener* mIotaMessageListener;
		std::string mGroupAlias;
		Poco::Path mFolderPath;
		Poco::SharedPtr<AddressIndex> mAddressIndex;
		model::files::State mGroupState;

		Poco::SharedPtr<model::gradido::GradidoBlock> mLastTransaction;
		int mLastAddressIndex;
		int mLastBlockNr;
		int mLastTransactionId;
		uint32_t mCoinColor;
		//std::list<BlockEntry> mBlocks;
		Poco::AccessExpireCache<Poco::UInt32, Block> mCachedBlocks;

		//! \brief get current block to write more transactions in it
		Poco::SharedPtr<Block> getCurrentBlock();
		Poco::SharedPtr<Block> getBlock(Poco::UInt32 blockNr);
		Poco::SharedPtr<Block> getBlockContainingTransaction(uint64_t transactionId);

		// for preventing double transactions
		// keep first 32 Byte of first signature of each transaction from last MILESTONES_BOOTSTRAP_COUNT * 1.5 minutes
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
		Poco::ExpireCache<HalfSignature, void*> mCachedSignatures;
		mutable Poco::FastMutex mSignatureCacheMutex;

		Poco::ExpireCache<iota::MessageId, uint64_t> mMessageIdTransactionNrCache;
		mutable Poco::FastMutex mMessageIdTransactionNrCacheMutex;

		// Community Server listening on new blocks for his group
		JsonRequest* mCommunityServer;
		bool mExitCalled;
	};
}

#endif //__GRADIDO_NODE_CONTROLLER_GROUP_H
