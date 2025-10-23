#ifndef __GRADIDO_NODE_HIERO_NODE_ADDRESS_H
#define __GRADIDO_NODE_HIERO_NODE_ADDRESS_H

#include "ServiceEndpoint.h"
#include "gradido_blockchain/data/hiero/AccountId.h"

namespace hiero {

	using NodeAddressMessage = message<
		string_field<"RSA_Pubkey", 4>,
		int64_field<"nodeId", 5>,
		message_field<"nodeAccountId", 6, gradido::interaction::deserialize::HieroAccountIdMessage>,
		bytes_field<"nodeCertHash", 7>,
		message_field<"serviceEndpoint", 8, ServiceEndpointMessage, repeated>,
		string_field<"description", 9>
	>;

	class NodeAddress
	{	
	public:
		NodeAddress();
		NodeAddress(const NodeAddressMessage& message);
		NodeAddress(
			const std::string rsaPubkey,
			int64_t nodeId,
			const AccountId& nodeAccountId,
			ConstBlockPtr nodeCertHash,
			const vector<ServiceEndpoint>& serviceEndpoints,
			const std::string description
		);
		~NodeAddress();
		
		NodeAddressMessage getMessage() const;

		const ServiceEndpoint& pickRandomEndpoint() const;

		inline const std::string& getRsaPubkey() const { return mRsaPubkey; }
        inline const int64_t& getNodeId() const { return mNodeId; }
        inline const AccountId& getNodeAccountId() const { return mNodeAccountId; }
        inline ConstBlockPtr getNodeCertHash() const { return mNodeCertHash; }
        inline const std::vector<ServiceEndpoint>& getServiceEndpoinst() const { return mServiceEndpoint; }
        inline const std::string& getDescription() const { return mDescription; }

	protected:
		string mRsaPubkey;
		int64_t mNodeId;
		AccountId mNodeAccountId;
		ConstBlockPtr mNodeCertHash;
		vector<ServiceEndpoint> mServiceEndpoint;
		string mDescription;
	};
}
#endif // __GRADIDO_NODE_HIERO_NODE_ADDRESS_H
