#ifndef __GRADIDO_NODE_CONTROLLER_GROUP_H
#define __GRADIDO_NODE_CONTROLLER_GROUP_H

#include "ControllerBase.h"
#include "AddressIndex.h"
#include "Block.h"
#include "BlockIndex.h"
#include "TaskObserver.h"

#include "../model/transactions/GradidoBlock.h"

#include "../model/files/GroupState.h"

#include "Poco/Path.h"
#include "Poco/AutoPtr.h"
#include "Poco/AccessExpireCache.h"



namespace controller {

	/*!
	* @author Dario Rekowski
	* @date 2020-02-06
	* @brief Main entry point for adding and searching for transactions
	*/

	class Group : public ControllerBase
	{
	public:
		//! \brief Load group states via model::files::GroupState.
		Group(std::string base58Hash, Poco::Path folderPath);
		~Group();

		//! \brief Put new transaction to block chain, if valid.
		//! \return True if valid, else false.
		bool addTransaction(Poco::AutoPtr<model::GradidoBlock> newTransaction);

		//! \brief Find last transaction of address account in memory or block chain.
		//! \param address Address = user account public key.
		Poco::AutoPtr<model::GradidoBlock> findLastTransaction(const std::string& address);
		//! \brief Find every transaction belonging to address account in memory or block chain, expensive.
		//!
		//! Use with care, can need some time and return huge amount of data.
		//! \param address Address = user account public key.
		std::vector<Poco::AutoPtr<model::GradidoBlock>> findTransactions(const std::string& address);
		//! \brief Find transactions of account in memory or block chain from a specific month.
		//! \param address User account public key.
		std::vector<Poco::AutoPtr<model::GradidoBlock>> findTransactions(const std::string& address, int month, int year);

		//! \brief Search for creation transactions from specific month and add balances together.
		//! \param address User account public key.
		uint64_t calculateCreationSum(const std::string& address, int month, int year);

		inline Poco::SharedPtr<AddressIndex> getAddressIndex() { return mAddressIndex; }

	protected:

		TaskObserver mTaskObserver;
		std::string mBase58GroupHash;
		Poco::Path mFolderPath;
		Poco::SharedPtr<AddressIndex> mAddressIndex;
		model::files::GroupState mGroupState;

		Poco::AutoPtr<model::GradidoBlock> mLastTransaction;
		int mLastAddressIndex;
		int mLastBlockNr;

		//std::list<BlockEntry> mBlocks;
		Poco::AccessExpireCache<Poco::UInt32, Block> mCachedBlocks;

		//! \brief get current block to write more transactions in it
		Poco::SharedPtr<Block> getCurrentBlock();
		Poco::SharedPtr<Block> getBlock(Poco::UInt32 blockNr);

	};
}

#endif //__GRADIDO_NODE_CONTROLLER_GROUP_H