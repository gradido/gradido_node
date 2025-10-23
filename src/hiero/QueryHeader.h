#ifndef __GRADIDO_NODE_HIERO_QUERY_HEADER_H
#define __GRADIDO_NODE_HIERO_QUERY_HEADER_H

#include "ResponseType.h"
#include "gradido_blockchain/interaction/deserialize/Protopuf.h"

namespace hiero {
	/**
	 * A standard query header.<br/>
	 * Each query from the client to the node must contain a QueryHeader, which
	 * specifies the desired response type, and includes a payment transaction
	 * that will compensate the network for responding to the query.
	 * The payment may be blank if the query is free.
	 *
	 * The payment transaction MUST be a `cryptoTransfer` from the payer account
	 * to the account of the node where the query is submitted.<br/>
	 * If the payment is sufficient, the network SHALL respond with the response
	 * type requested.<br/>
	 * If the response type is `COST_ANSWER` the payment MUST be unset.
	 * A state proof SHALL be available for some types of information.<br/>
	 * A state proof SHALL be available for a Record, but not a receipt, and the
	 * response entry for each supported "get info" query.
	 */
	using QueryHeaderMessage = message<
		// only needed for queries with fees
		// message_field<"payment", 1, Transaction>,
		enum_field<"responseType", 2, ResponseType>
	>;
	class QueryHeader
	{
	public:
		QueryHeader();
		QueryHeader(ResponseType type);
		QueryHeader(const QueryHeaderMessage& message);
		~QueryHeader();

		QueryHeaderMessage getMessage() const;
		inline ResponseType getResponseType() const { return mResponseType; }

		static constexpr size_t maxSize() { return 2; }

	protected:
		ResponseType mResponseType;
	};
}

#endif //__GRADIDO_NODE_HIERO_QUERY_HEADER_H