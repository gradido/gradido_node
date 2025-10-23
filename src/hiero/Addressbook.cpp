#include "../SystemExceptions.h"
#include "Addressbook.h"
#include "gradido_blockchain/lib/Profiler.h"

#include "loguru/loguru.hpp"

#include <fstream>

namespace hiero {	

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
		auto result = message_coder<NodeAddressBookMessage>::decode(buffer.span());
		if (!result.has_value()) {
			throw GradidoNodeInvalidDataException("invalid addressbook format");
		}
		const auto& [message, bufferEnd2] = *result;
		mNodeAddressBook = NodeAddressBook(message);
		
		const auto& nodeAddresses = mNodeAddressBook.getNodeAddresses();

		if (!nodeAddresses.size()) {
			LOG_F(INFO, "%s is empty, no server addresses found", mFilePath.data());
		}
		LOG_F(INFO, "time for deserialize %d addresses: %s", (int)nodeAddresses.size(), timeUsed.string().data());
	}
}
