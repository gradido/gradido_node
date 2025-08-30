#include "ResponseHeader.h"

namespace hiero {
	ResponseHeader::ResponseHeader()
	{

	}
	ResponseHeader::ResponseHeader(const ResponseHeaderMessage& message)
	{
		mNodeTransactionPrecheckCode = message["nodeTransactionPrecheckCode"_f].value();
		mResponseType = message["responseType"_f].value();
		mCost = message["cost"_f].value();
		if (message["stateProof"_f].has_value()) {
			mStateProof = std::make_shared<const memory::Block>(message["stateProof"_f].value());
		}
	}
	ResponseHeader::ResponseHeader(
		ResponseCodeEnum nodeTransactionPrecheckCode,
		ResponseType responseType,
		uint64_t cost,
		memory::ConstBlockPtr stateProof /*= nullptr*/
	) : mNodeTransactionPrecheckCode(nodeTransactionPrecheckCode),
		mResponseType(responseType),
		mCost(cost),
		mStateProof(stateProof)
	{

	}

	ResponseHeader::~ResponseHeader()
	{

	}

	ResponseHeaderMessage ResponseHeader::getMessage() const
	{
		ResponseHeaderMessage message;
		message["nodeTransactionPrecheckCode"_f] = mNodeTransactionPrecheckCode;
		message["responseType"_f] = mResponseType;
		message["cost"_f] = mCost;
		if (mStateProof) {
			message["stateProof"_f] = mStateProof->copyAsVector();
		}
		return message;
	}
}