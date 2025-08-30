#ifndef __GRADIDO_NODE_HIERO_RESPONSE_HEADER_H
#define __GRADIDO_NODE_HIERO_RESPONSE_HEADER_H

#include "ResponseCodeEnum.h"
#include "ResponseType.h"
#include "gradido_blockchain/interaction/deserialize/Protopuf.h"
#include "gradido_blockchain/memory/Block.h"

namespace hiero {
	using ResponseHeaderMessage = message<
		enum_field<"nodeTransactionPrecheckCode", 1, ResponseCodeEnum>,
		enum_field<"responseType", 2, ResponseType>,
		uint64_field<"cost", 3>,
		bytes_field<"stateProof", 4>
	>;
	class ResponseHeader
	{
	public:
		ResponseHeader();
		ResponseHeader(const ResponseHeaderMessage& message);
		ResponseHeader(
			ResponseCodeEnum nodeTransactionPrecheckCode,
			ResponseType responseType,
			uint64_t cost,
			memory::ConstBlockPtr stateProof = nullptr
		);
		~ResponseHeader();

		ResponseHeaderMessage getMessage() const;
		inline ResponseCodeEnum getNodeTransactionPrecheckCode() const { return mNodeTransactionPrecheckCode; }
		inline ResponseType getResponseType() const { return mResponseType; }
		inline uint64_t getCost() const { return mCost; }
		inline memory::ConstBlockPtr getStateProof() const { return mStateProof; }

	protected:
		ResponseCodeEnum mNodeTransactionPrecheckCode;
		ResponseType mResponseType;
		uint64_t mCost;
		memory::ConstBlockPtr mStateProof;

	};
}

#endif //__GRADIDO_NODE_HIERO_RESPONSE_HEADER_H