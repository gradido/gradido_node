#include "NodeAddress.h"
#include "gradido_blockchain/interaction/deserialize/HieroAccountIdRole.h"
#include "gradido_blockchain/interaction/serialize/HieroAccountIdRole.h"
#include "loguru/loguru.hpp"

using namespace gradido::interaction;

namespace hiero {
	NodeAddress::NodeAddress(): mNodeId(0), mNodeCertHash(0)
	{
	}

    NodeAddress::NodeAddress(
        const std::string rsaPubkey,
        int64_t nodeId,
        const AccountId& nodeAccountId,
        ConstBlockPtr nodeCertHash,
        const vector<ServiceEndpoint>& serviceEndpoints,
        const std::string description
    ) : mRsaPubkey(rsaPubkey), 
        mNodeId(nodeId), 
        mNodeAccountId(nodeAccountId), 
        mNodeCertHash(nodeCertHash), 
        mServiceEndpoint(serviceEndpoints), 
        mDescription(description)
    {
    }

	NodeAddress::NodeAddress(const NodeAddressMessage& message)
		: mNodeId(0), mNodeCertHash(0)
	{		
		mRsaPubkey = message["RSA_Pubkey"_f].value();
		if (message["nodeId"_f].has_value()) {
			mNodeId = message["nodeId"_f].value();
		}
		mNodeAccountId = deserialize::HieroAccountIdRole(message["nodeAccountId"_f].value());
		auto temp = Block::fromHex(Block(message["nodeCertHash"_f].value()).copyAsString());
		mNodeCertHash = std::make_shared<const Block>(temp);
		auto serviceEndpointMessages = message["serviceEndpoint"_f];
		mServiceEndpoint.reserve(serviceEndpointMessages.size());
		for (int j = 0; j < message["serviceEndpoint"_f].size(); j++) {
			mServiceEndpoint.push_back(ServiceEndpoint(serviceEndpointMessages[j]));
		}
		mDescription = message["description"_f].value();		
	}

	NodeAddress::~NodeAddress()
	{

	}

	NodeAddressMessage NodeAddress::getMessage() const
	{
		NodeAddressMessage message;
		message["RSA_Pubkey"_f] = mRsaPubkey;
		if (mNodeId) {
			message["nodeId"_f] = mNodeId;
		}
		auto serializeAccountId = serialize::HieroAccountIdRole(mNodeAccountId);
		message["nodeAccountId"_f].value() = serializeAccountId.getMessage();
		message["nodeCertHash"_f] = mNodeCertHash->copyAsVector();
		message["serviceEndpoint"_f].reserve(mServiceEndpoint.size());
		for (const auto& serviceEndpoint : mServiceEndpoint) {
			message["serviceEndpoint"_f].push_back(serviceEndpoint.getMessage());
		}
		message["description"_f] = mDescription;

		return message;
	}

	const ServiceEndpoint& NodeAddress::pickRandomEndpoint() const
	{
		size_t randomIndex = std::rand() % mServiceEndpoint.size();
		if (randomIndex >= mServiceEndpoint.size()) {
			LOG_F(FATAL, "random generator don't work like expected");
		}
		return mServiceEndpoint[randomIndex];
	}	
}