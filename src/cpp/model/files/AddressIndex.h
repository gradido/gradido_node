#ifndef __GRADIDO_NODE_MODEL_FILES_ADDRESS_INDEX_H
#define __GRADIDO_NODE_MODEL_FILES_ADDRESS_INDEX_H

#include "FileBase.h"
#include "../../SingletonManager/MemoryManager.h"
#include <unordered_map>

namespace model {
	namespace files {
		class AddressIndex : public FileBase
		{
		public:
			AddressIndex(Poco::Path filePath);
			~AddressIndex();

			bool addAddressIndex(const std::string& address, uint32_t index);
			uint32_t getIndexForAddress(const std::string &address);

			void safeExit();

		protected:
			// check if index file contains current indices (compare sizes)
			bool checkFile();
			size_t calculateFileSize();

			static MemoryBin* calculateHash(const std::map<int, std::string>& sortedMap);

			bool loadFromFile();

			Poco::Path mFilePath;
			std::unordered_map<std::string, uint32_t> mAddressesIndices;
			bool mFileWritten;
		};
	}
}

#endif 