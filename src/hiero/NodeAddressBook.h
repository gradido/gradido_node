#ifndef __GRADIDO_NODE_HIERO_NODE_ADDRESS_BOOK_H
#define __GRADIDO_NODE_HIERO_NODE_ADDRESS_BOOK_H

#include "NodeAddress.h"

namespace hiero {

	using NodeAddressBookMessage = message<
		message_field<"nodeAddress", 1, NodeAddressMessage, repeated>
	>;

	class NodeAddressBook
	{
	public:
		NodeAddressBook();
		NodeAddressBook(const NodeAddressBookMessage& message);
		NodeAddressBook(const std::vector<NodeAddress>& nodeAddresses);
		~NodeAddressBook();

		NodeAddressBookMessage getMessage();
		const NodeAddress& pickRandomNode() const;
		inline const std::vector<NodeAddress>& getNodeAddresses() const { return mNodeAddresses; }

	protected:
		std::vector<NodeAddress> mNodeAddresses;
	};
}
#endif // __GRADIDO_NODE_HIERO_NODE_ADDRESS_BOOK_H