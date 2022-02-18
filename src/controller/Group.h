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
		Group(std::string alias, Poco::Path folderPath);
		~Group();

		// initialize, fill cache
		bool init();
		// clean up group, stopp all running processes
		void exit();

		//! \brief Put new transaction to block chain, if valid.
		//! \return True if valid, else false.
		bool addTransaction(std::shared_ptr<model::gradido::GradidoBlock> newTransaction);

		
		//! \brief add transaction from iota into blockchain, important! must be called in correct order
		bool addTransactionFromIota(std::shared_ptr<model::gradido::GradidoTransaction> newTransaction, uint32_t iotaMilestoneId, uint64_t iotaMilestoneTimestamp);

		//! \brief Find last transaction of address account in memory or block chain.
		//! \param address Address = user account public key.
		Poco::AutoPtr<model::gradido::GradidoBlock> findLastTransaction(const std::string& address);

		std::shared_ptr<model::gradido::GradidoBlock> getLastTransaction();

		std::vector<std::string> findTransactionsSerialized(uint64_t fromTransactionId);
		//! \brief Find every transaction belonging to address account in memory or block chain, expensive.
		//!
		//! Use with care, can need some time and return huge amount of data.
		//! \param address Address = user account public key.
		std::vector<Poco::AutoPtr<model::gradido::GradidoBlock>> findTransactions(const std::string& address);
		//! \brief Find transactions of account in memory or block chain from a specific month.
		//! \param address User account public key.
		std::vector<Poco::AutoPtr<model::gradido::GradidoBlock>> findTransactions(const std::string& address, int month, int year);

		//! \brief Search for creation transactions from specific month and add balances together.
		//! \param address User account public key.
		uint64_t calculateCreationSum(const std::string& address, int month, int year, Poco::DateTime received);

		inline Poco::SharedPtr<AddressIndex> getAddressIndex() { return mAddressIndex; }

		inline const std::string& getGroupAlias() { return mGroupAlias; }

		bool isSignatureInCache(Poco::AutoPtr<model::gradido::GradidoTransaction> transaction);

		void setListeningCommunityServer(Poco::URI uri);

	protected:
		void updateLastAddressIndex(int lastAddressIndex);
		void updateLastBlockNr(int lastBlockNr);
		void updateLastTransactionId(int lastTransactionId);

		bool isTransactionAlreadyExist(Poco::AutoPtr<model::gradido::GradidoTransaction> transaction);
		void addSignatureToCache(Poco::AutoPtr<model::gradido::GradidoTransaction> transaction);
		//! read blocks starting by latest until block is older than MILESTONES_BOOTSTRAP_COUNT * 1.5 minutes | io read expensive
		//! put all signatures from young enough blocks into signature cache
		void fillSignatureCacheOnStartup();

		TaskObserver mTaskObserver;
		iota::MessageListener* mIotaMessageListener;
		std::string mGroupAlias;
		Poco::Path mFolderPath;
		Poco::SharedPtr<AddressIndex> mAddressIndex;
		model::files::State mGroupState;

		std::shared_ptr<model::gradido::GradidoBlock> mLastTransaction;
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
		// keep first 32 Byte of first signature of each transaction from last MILESTONES_BOOTSTRAP_COUNT * 1.5 minutes
		struct HalfSignature
		{
			HalfSignature(const char* signature) {
				memcpy(&sign, signature, 32);
			}
			HalfSignature(Poco::AutoPtr<model::gradido::GradidoTransaction> transaction) {
				auto sigPairs = transaction->getSigMap().sigpair();
				if (sigPairs.size() == 0) {
					throw std::runtime_error("[Group::addSignatureToCache] empty signatures");
				}
				memcpy(&sign, sigPairs.Get(0).signature().data(), 32);
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
		Poco::FastMutex mSignatureCacheMutex;
		// Community Server listening on new blocks for his group
		JsonRequest* mCommunityServer;
		bool mExitCalled;
	};
}

#endif //__GRADIDO_NODE_CONTROLLER_GROUP_H
