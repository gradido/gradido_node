#ifndef __GRADIDO_NODE_CACHE_MESSAGE_ID_H
#define __GRADIDO_NODE_CACHE_MESSAGE_ID_H

#include "../model/files/State.h"
#include "gradido_blockchain/lib/ExpireCache.h"
#include "gradido_blockchain/data/iota/MessageId.h"

namespace cache
{
	class MessageId 
	{
	public:
		MessageId(std::string_view folder);
		~MessageId();

		// try to open db 
		bool init();
		void exit();
		//! remove state level db folder, clear maps
		void reset();

		void add(memory::ConstBlockPtr messageId, uint64_t transactionNr);
		bool has(memory::ConstBlockPtr messageId);
		//! \return 0 if not found, else transaction nr for message id
		uint64_t getTransactionNrForMessageId(memory::ConstBlockPtr messageId);

	protected:
		//! read message id as key from level db and put into mMessageIdTransactionNrs if found
		//! \return 0 if not found or else transactionNr for message Id
		uint64_t readFromLevelDb(const iota::MessageId& iotaMessageIdObj);

		bool mInitalized;
		model::files::State mStateFile;
		//! key is iota message id, value is transaction nr
		ExpireCache<iota::MessageId, uint64_t> mMessageIdTransactionNrs;
	};
}

#endif //__GRADIDO_NODE_CACHE_MESSAGE_ID_H