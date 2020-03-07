#ifndef __GRADIDO_NODE_CONTROLLER_GROUP_H
#define __GRADIDO_NODE_CONTROLLER_GROUP_H

#include "ControllerBase.h"
#include "AddressIndex.h"
#include "Block.h"
#include "BlockIndex.h"

#include "../model/transactions/Transaction.h"

#include "../model/files/GroupState.h"

#include "Poco/Path.h"
#include "Poco/AutoPtr.h"
#include "Poco/AccessExpireCache.h"



namespace controller {

	class Group : public ControllerBase
	{
	public:
		Group(std::string base58Hash, Poco::Path folderPath);
		~Group();

		// put new transaction at blockchain, if valid
		//! \return true if valid, else false
		bool addTransaction(Poco::AutoPtr<model::Transaction> newTransaction);

		//!  \param address = public key
		Poco::AutoPtr<model::Transaction> findLastTransaction(const std::string& address);
		std::vector<Poco::AutoPtr<model::Transaction>> findTransactions(const std::string& address);
		std::vector<Poco::AutoPtr<model::Transaction>> findTransactions(const std::string& address, int month, int year);

		uint64_t calculateCreationSum(const std::string& address, int month, int year);

	protected:

		std::string mBase58GroupHash;
		Poco::Path mFolderPath;
		AddressIndex mAddressIndex;
		model::files::GroupState mGroupState;

		Poco::AutoPtr<model::Transaction> mLastTransaction;
		int mLastKtoKey;
		int mLastBlockNr;

		struct BlockEntry 
		{
			BlockEntry(Poco::UInt32 blockNr);

			Poco::AutoPtr<Block> block;
			Poco::AutoPtr<BlockIndex> blockIndex;
			Poco::UInt32 blockNr;
		};

		//std::list<BlockEntry> mBlocks;
		Poco::AccessExpireCache<Poco::UInt32, BlockEntry> mCachedBlocks;

		Poco::SharedPtr<BlockEntry> getBlockEntry(Poco::UInt32 blockNr);

	};
}

#endif //__GRADIDO_NODE_CONTROLLER_GROUP_H