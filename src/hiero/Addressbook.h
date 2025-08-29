#ifndef __GRADIDO_NODE_CLIENT_GRPC_ADDRESSBOOK_H
#define __GRADIDO_NODE_CLIENT_GRPC_ADDRESSBOOK_H

#include "gradido_blockchain/types.h"
#include "gradido_blockchain/memory/Block.h"
#include "gradido_blockchain/data/hiero/AccountId.h"
#include "NodeAddressBook.h"

namespace hiero {
	
	class Addressbook 
	{
	public:
		Addressbook(const char* filePath);
		~Addressbook();

		// load node addresses from <networkType>.pb file
		void load();
		inline const ServiceEndpoint& pickRandomEndpoint() const {
			return mNodeAddressBook.pickRandomNode().pickRandomEndpoint();
		}

	protected:
		std::string mFilePath;
		NodeAddressBook mNodeAddressBook;
	};
}

#endif //__GRADIDO_NODE_CLIENT_GRPC_ADDRESSBOOK_H