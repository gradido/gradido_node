#include "AddressIndex.h"
#include "sodium.h"
#include <map>

#include "Poco/File.h"
#include "Poco/FileStream.h"

#include "../../SingletonManager/FileLockManager.h"

namespace model {
	namespace files {

		AddressIndex::AddressIndex(Poco::Path filePath)
			: mFilePath(filePath), mFileWritten(false)
		{
			loadFromFile();
		}

		AddressIndex::~AddressIndex()
		{
			if (!mFileWritten) {
				safeExit();
			}
		}

		bool AddressIndex::addAddressIndex(const std::string& address, uint32_t index)
		{
			Poco::FastMutex::ScopedLock lock(mFastMutex);

			mFileWritten = false;
			auto result = mAddressesIndices.insert(std::pair<std::string, uint32_t>(address, index));
			return result.second;
		}

		uint32_t AddressIndex::getIndexForAddress(const std::string &address)
		{
			Poco::FastMutex::ScopedLock lock(mFastMutex);

			auto it = mAddressesIndices.find(address);
			if (it != mAddressesIndices.end()) {
				return it->second;
			}
			return 0;
		}

		bool AddressIndex::checkFile()
		{
			auto fl = FileLockManager::getInstance();
			int timeoutRounds = 100;
			auto filePath = mFilePath.toString();
			if (!fl->tryLockTimeout(filePath, timeoutRounds, this)) {
				return false;
			}

			Poco::File file(mFilePath);
			if (file.exists() && file.canRead()) {
				if (file.getSize() == calculateFileSize()) {
					fl->unlock(filePath);
					return true;
				}
			}
			fl->unlock(filePath);
			return false;
		}

		size_t AddressIndex::calculateFileSize()
		{
			Poco::FastMutex::ScopedLock lock(mFastMutex);

			auto mapEntryCount = mAddressesIndices.size();
			return mapEntryCount * (sizeof(uint32_t) + 32) + 32;
		}

		MemoryBin* AddressIndex::calculateHash(const std::map<int, std::string>& sortedMap)
		{
			auto mm = MemoryManager::getInstance();
			auto hash = mm->getFreeMemory(crypto_generichash_BYTES);

			crypto_generichash_state state;
			crypto_generichash_init(&state, nullptr, 0, crypto_generichash_BYTES);
			
			for (auto it = sortedMap.begin(); it != sortedMap.end(); it++) {
				std::string line = it->second + std::to_string(it->first);
				crypto_generichash_update(&state, (const unsigned char*)line.data(), line.size());
			}

			crypto_generichash_final(&state, *hash, hash->size());

			return hash;
		}

		void AddressIndex::safeExit()
		{
			if (checkFile() || errorCount()) {
				// current file seems containing address indices 
				mFileWritten = true;
				return;
			}
			auto mm = MemoryManager::getInstance();
			auto fl = FileLockManager::getInstance();
			Poco::FastMutex::ScopedLock lock(mFastMutex);
			std::map<int, std::string> sortedMap;
			auto filePath = mFilePath.toString();
			if (!fl->tryLockTimeout(filePath, 100, this)) {
				return;
			}
			Poco::FileOutputStream file(filePath);

			for (auto it = mAddressesIndices.begin(); it != mAddressesIndices.end(); it++) {
				auto inserted = sortedMap.insert(std::pair<int, std::string>(it->second, it->first));
				if (!inserted.second) {
					addError(new Error(__FUNCTION__, "inserting index address pair failed, index already exist!"));
					std::string currentPair = std::to_string(it->second) + ": " + it->first;
					addError(new ParamError(__FUNCTION__, "current pair", currentPair.data()));
					std::string lastPair = std::to_string(inserted.first->first) + ": " + inserted.first->second;
					addError(new ParamError(__FUNCTION__, "last pair", lastPair.data()));
					throw new Poco::Exception("[model::files::AddressIndex] inserting index address pair failed, index already exist!");
				}
				file << it->first << it->second;
			}
			auto hash = calculateHash(sortedMap);
			file << hash;
			file.close();
			fl->unlock(filePath);

			mm->releaseMemory(hash);			
			mFileWritten = true;
		}

		bool AddressIndex::loadFromFile()
		{
			auto fl = FileLockManager::getInstance();
			auto mm = MemoryManager::getInstance();
			int timeoutRounds = 100;
			auto filePath = mFilePath.toString();
			if (!fl->tryLockTimeout(filePath, timeoutRounds, this)) {
				return false;
			}
			Poco::File file(filePath);
			if (!file.exists()) {
				fl->unlock(filePath);
				return false;
			}

			auto fileSize = file.getSize();
			auto readedSize = 0;
			Poco::FileInputStream fileStream(filePath);
			auto hash = mm->getFreeMemory(32);
			uint32_t index = 0;
			std::map<int, std::string> sortedMap;

			while (readedSize < fileSize) {
				fileStream.read(*hash, 32);
				readedSize += 32;
				if (readedSize + sizeof(uint32_t) + 32 <= fileSize) {
					fileStream.read((char*)&index, sizeof(uint32_t));
					readedSize += sizeof(uint32_t);
					std::string hashString((const char*)*hash, hash->size());
					mAddressesIndices.insert(std::pair<std::string, int>(hashString, index));
					sortedMap.insert(std::pair<int, std::string>(index, hashString));
				}
				else {
					break;
				}
			}
			auto compareHash = calculateHash(sortedMap);
			auto compareResult = memcmp(*hash, *compareHash, 32);
			fl->unlock(filePath);
			mm->releaseMemory(hash);
			mm->releaseMemory(compareHash);

			if (compareResult != 0) {
				addError(new ParamError(__FUNCTION__, "file hash mismatch", filePath.data()));
				return false;
			}

			return true;
		}
	}

}