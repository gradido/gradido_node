#include "NodeAddressBook.h"
#include "loguru/loguru.hpp"

namespace hiero {
	NodeAddressBook::NodeAddressBook()
	{

	}

	NodeAddressBook::NodeAddressBook(const NodeAddressBookMessage& message)
	{
		auto nodeAddresses = message["nodeAddress"_f];
		if (nodeAddresses.empty()) return; 
		mNodeAddresses.reserve(nodeAddresses.size());
		for (int i = 0; i < nodeAddresses.size(); i++) {
			mNodeAddresses.push_back(NodeAddress(nodeAddresses[i]));
		}
	}

	NodeAddressBook::NodeAddressBook(const std::vector<NodeAddress>& nodeAddresses)
		: mNodeAddresses(nodeAddresses)
	{

	}


	NodeAddressBook::~NodeAddressBook()
	{

	}

	NodeAddressBookMessage NodeAddressBook::getMessage()
	{
		NodeAddressBookMessage message;
		if (mNodeAddresses.empty()) return message;

		message["nodeAddress"_f].reserve(mNodeAddresses.size());
		for (const auto& nodeAddress : mNodeAddresses) {
			message["nodeAddress"_f].push_back(nodeAddress.getMessage());
		}
		return message;
	}

	const NodeAddress& NodeAddressBook::pickRandomNode() const
	{
		size_t randomIndex = std::rand() % mNodeAddresses.size();
		if (randomIndex >= mNodeAddresses.size()) {
			LOG_F(FATAL, "random generator don't work like expected");
		}
		return mNodeAddresses[randomIndex];
	}

}