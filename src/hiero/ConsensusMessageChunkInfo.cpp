#include "ConsensusMessageChunkInfo.h"
#include "gradido_blockchain/interaction/deserialize/HieroTransactionIdRole.h"
#include "gradido_blockchain/interaction/serialize/HieroTransactionIdRole.h"

using namespace gradido::interaction;

namespace hiero {
	ConsensusMessageChunkInfo::ConsensusMessageChunkInfo()
		: mTotal(0), mNumber(0)
	{

	}

	ConsensusMessageChunkInfo::ConsensusMessageChunkInfo(const ConsensusMessageChunkInfoMessage& message)
	{
		mInitialTransactionID = deserialize::HieroTransactionIdRole(message["initialTransactionID"_f].value());
		mTotal = message["total"_f].value();
		mNumber = message["number"_f].value();
	}

	ConsensusMessageChunkInfo::ConsensusMessageChunkInfo(const TransactionId& initialTransactionId, int32_t total, int32_t number)
		: mInitialTransactionID(initialTransactionId), mTotal(total), mNumber(number)
	{

	}

	ConsensusMessageChunkInfo::~ConsensusMessageChunkInfo()
	{

	}

	ConsensusMessageChunkInfoMessage ConsensusMessageChunkInfo::getMessage() const
	{
		ConsensusMessageChunkInfoMessage message;
		message["initialTransactionID"_f] = serialize::HieroTransactionIdRole(mInitialTransactionID).getMessage();
		message["total"_f] = mTotal;
		message["number"_f] = mNumber;
		return message;
	}
}