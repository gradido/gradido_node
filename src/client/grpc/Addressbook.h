#ifndef __GRADIDO_NODE_CLIENT_GRPC_ADDRESSBOOK_H
#define __GRADIDO_NODE_CLIENT_GRPC_ADDRESSBOOK_H

#include "gradido_blockchain/types.h"
#include "gradido_blockchain/memory/Block.h"
#include "gradido_blockchain/data/hiero/AccountId.h"

namespace client {
	namespace grpc {
		struct ServiceEndpoint
		{
			ServiceEndpoint() : ipAddressV4(0), port(0) {};
			std::string getConnectionString() const;

			/**
			 * A 32-bit IPv4 address.<br/>
			 * This is the address of the endpoint, encoded in pure "big-endian"
			 * (i.e. left to right) order (e.g. `127.0.0.1` has hex bytes in the
			 * order `7F`, `00`, `00`, `01`).
			 */
			memory::Block ipAddressV4;

			/**
			 * A TCP port to use.
			 * <p>
			 * This value MUST be between 0 and 65535, inclusive.
			 */
			int32_t port;

			/**
			 * A node domain name.
			 * <p>
			 * This MUST be the fully qualified domain name of the node.<br/>
			 * This value MUST NOT exceed 253 characters.<br/>
			 * When the `domain_name` field is set, the `ipAddressV4`
			 * field MUST NOT be set.<br/>
			 * When the `ipAddressV4` field is set, the `domain_name`
			 * field MUST NOT be set.
			 */
			std::string domain;
		};

		struct NodeAddress
		{
			NodeAddress(): nodeCertHash(0), nodeId(0) {}
			const ServiceEndpoint& pickRandomEndpoint() const;
			std::string rsaPubkey;
			int64_t nodeId;
			hiero::AccountId nodeAccountId;
			memory::Block nodeCertHash;
			std::vector<ServiceEndpoint> serviceEndpoint;
			std::string description;
		};

		class Addressbook 
		{
		public:
			Addressbook(const char* filePath);
			~Addressbook();

			// load node addresses from <networkType>.pb file
			void load();
			const NodeAddress& pickRandomNode() const;
			inline const ServiceEndpoint& pickRandomEndpoint() const {
				return pickRandomNode().pickRandomEndpoint();
			}

		protected:
			std::string mFilePath;
			std::vector<NodeAddress> mNodeAddresses;
		};
	}
}

#endif //__GRADIDO_NODE_CLIENT_GRPC_ADDRESSBOOK_H