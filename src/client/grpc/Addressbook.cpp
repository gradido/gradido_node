#include "../../SystemExceptions.h"
#include "Addressbook.h"
#include "gradido_blockchain/interaction/deserialize/Protopuf.h"
#include "gradido_blockchain/lib/Profiler.h"

#include "loguru/loguru.hpp"

#include <fstream>

using namespace pp;
using namespace gradido::interaction::deserialize;

using ServiceEndpointMessage = message<
	bytes_field<"ipAddressV4", 1>,
	int32_field<"port", 2>,
	string_field<"domain_name", 3>
>;

using NodeAddressMessage = message<
	string_field<"RSA_Pubkey", 4>,
	int64_field<"nodeId", 5>,
	message_field<"nodeAccountId", 6, HieroAccountIdMessage>,
	bytes_field<"nodeCertHash", 7>,
	message_field<"serviceEndpoint", 8, ServiceEndpointMessage, repeated>,
	string_field<"description", 9>
>;

using NodeAddressBook = message<
	message_field<"nodeAddress", 1, NodeAddressMessage, repeated>
>;


namespace client {
	namespace grpc {
		std::string ServiceEndpoint::getConnectionString() const
		{
			std::string url;
			if (!ipAddressV4.isEmpty()) {
				auto v = ipAddressV4.copyAsVector();
				for (int i = 0; i < v.size(); i++) {
					if (i) {
						url += '.';
					}
					url += std::to_string(v[i]);
				}
			}
			else {
				url = domain;
			}
			url += ':';
			url += std::to_string(port);
			return url;
		}

		const ServiceEndpoint& NodeAddress::pickRandomEndpoint() const
		{
			size_t randomIndex = std::rand() % serviceEndpoint.size();
			if (randomIndex >= serviceEndpoint.size()) {
				LOG_F(FATAL, "random generator don't work like expected");
			}
			return serviceEndpoint[randomIndex];
		}

		Addressbook::Addressbook(const char* filePath)
			: mFilePath(filePath)
		{

		}

		Addressbook::~Addressbook()
		{

		}

		void Addressbook::load()
		{
			Profiler timeUsed;
			mNodeAddresses.clear();

			// read addressbook from binary file
			std::ifstream file(mFilePath, std::ios::binary | std::ios::ate);
			if (!file.is_open()) {
				throw FileNotFoundException("couldn't open hiero addressbook file", mFilePath.data());
			}
			size_t fileSize = file.tellg();
			file.seekg(0, std::ios::beg);
			memory::Block buffer(fileSize);
			file.read((char*)buffer.data(), fileSize);
			file.close();

			// deserialize
			auto result = message_coder<NodeAddressBook>::decode(buffer.span());
			if (!result.has_value()) {
				throw GradidoNodeInvalidDataException("invalid addressbook format");
			}
			const auto& [message, bufferEnd2] = *result;
			auto nodeAddresses = message["nodeAddress"_f];
			if (!nodeAddresses.size()) {
				LOG_F(INFO, "%s is empty, no server addresses found", mFilePath.data());
			}
			mNodeAddresses.reserve(nodeAddresses.size());
			for (int i = 0; i < nodeAddresses.size(); i++) {
				auto m = nodeAddresses[i];
				NodeAddress nodeAddress;
				nodeAddress.rsaPubkey = m["RSA_Pubkey"_f].value();
				if (m["nodeId"_f].has_value()) {
					nodeAddress.nodeId = m["nodeId"_f].value();
				}
				auto nodeAccountIdMessage = m["nodeAccountId"_f].value();
				int64_t shardNum = 0;
				int64_t realmNum = 0;
				if (nodeAccountIdMessage["shardNum"_f].has_value()) {
					shardNum = nodeAccountIdMessage["shardNum"_f].value();
				}
				if (nodeAccountIdMessage["realmNum"_f].has_value()) {
					realmNum = nodeAccountIdMessage["realmNum"_f].value();
				}
				if (nodeAccountIdMessage["accountNum"_f].has_value()) {
					nodeAddress.nodeAccountId = hiero::AccountId(shardNum, realmNum, nodeAccountIdMessage["accountNum"_f].value());
				}
				else if (nodeAccountIdMessage["alias"_f].has_value()) {
					auto alias = memory::Block(nodeAccountIdMessage["alias"_f].value());
					nodeAddress.nodeAccountId = hiero::AccountId(shardNum, realmNum, std::move(alias));
				}
				else {
					throw GradidoNodeInvalidDataException("either accountNum or alias must be set, invalid format for nodeAccountId (AccountId)");
				}
				nodeAddress.nodeCertHash = m["nodeCertHash"_f].value();
				auto serviceEndpointMessages = m["serviceEndpoint"_f];
				nodeAddress.serviceEndpoint.reserve(serviceEndpointMessages.size());
				for (int j = 0; j < m["serviceEndpoint"_f].size(); j++) {
					auto s = serviceEndpointMessages[j];
					ServiceEndpoint serviceEndpoint;
					if (s["ipAddressV4"_f].has_value()) {
						serviceEndpoint.ipAddressV4 = memory::Block(s["ipAddressV4"_f].value());
					}
					else {
						serviceEndpoint.domain = s["domain_name"_f].value();
					}
					serviceEndpoint.port = s["port"_f].value();					
					nodeAddress.serviceEndpoint.push_back(serviceEndpoint);
				}
				nodeAddress.description = m["description"_f].value();
				mNodeAddresses.push_back(nodeAddress);
			}
			LOG_F(INFO, "time for deserialize %d addresses: %s", (int)mNodeAddresses.size(), timeUsed.string().data());
		}

		const NodeAddress& Addressbook::pickRandomNode() const
		{	
			size_t randomIndex = std::rand() % mNodeAddresses.size();
			if (randomIndex >= mNodeAddresses.size()) {
				LOG_F(FATAL, "random generator don't work like expected");
			}
			return mNodeAddresses[randomIndex];
		}
	}
}