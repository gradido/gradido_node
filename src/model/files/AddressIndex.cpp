#include "AddressIndex.h"
#include "FileExceptions.h"
#include "sodium.h"
#include <map>

#include "Poco/File.h"
#include "Poco/FileStream.h"
#include "Poco/Timespan.h"

#include "../../SingletonManager/FileLockManager.h"
#include "../../ServerGlobals.h"

namespace model {
	namespace files {

		AddressIndex::AddressIndex(Poco::Path filePath)
			: mFilePath(filePath), mFileWritten(false)
		{
			if (loadFromFile()) {
				mFileWritten = true;
			}
		}

		AddressIndex::~AddressIndex()
		{
			if (!isFileWritten()) {
				writeToFile();
			}
		}

		bool AddressIndex::add(const std::string& address, uint32_t index)
		{
			if (address == "") {
				throw std::runtime_error("empty address");
			}
			mFileWritten = false;
			mFastMutex.lock();
			bool result = mAddressesIndices.insert(std::pair<std::string, uint32_t>(address, index)).second;
			mFastMutex.unlock();

			return result;
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
			if (!fl->tryLockTimeout(filePath, timeoutRounds)) {
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

		void AddressIndex::setFileWritten()
		{
			Poco::FastMutex::ScopedLock lock(mFastMutex);
			mFileWritten = true;
		}

		size_t AddressIndex::calculateFileSize()
		{
			Poco::FastMutex::ScopedLock lock(mFastMutex);

			auto mapEntryCount = mAddressesIndices.size();
			return mapEntryCount * (sizeof(uint32_t) + 32) + 32;
		}

		MemoryBin AddressIndex::calculateHash(const std::map<int, std::string>& sortedMap)
		{
			MemoryBin hash(crypto_generichash_BYTES);

			crypto_generichash_state state;
			crypto_generichash_init(&state, nullptr, 0, crypto_generichash_BYTES);
			
			for (auto it = sortedMap.begin(); it != sortedMap.end(); it++) {
				std::string line = it->second + std::to_string(it->first);
				crypto_generichash_update(&state, (const unsigned char*)line.data(), line.size());
			}

			crypto_generichash_final(&state, hash.data(), hash.size());

			return hash;
		}

		void AddressIndex::writeToFile()
		{
			if (checkFile()) {
				// current file seems containing address indices 
				mFileWritten = true;
				return;
			}
			auto fl = FileLockManager::getInstance();
			Poco::FastMutex::ScopedLock lock(mFastMutex);
			std::map<int, std::string> sortedMap;
			auto filePath = mFilePath.toString();
			if (!fl->tryLockTimeout(filePath, 100)) {
				return;
			}
			// check if dir exist, if not create it
			Poco::Path pathDir = mFilePath;
			pathDir.makeDirectory().popDirectory();
			Poco::File fileFolder(pathDir);
			fileFolder.createDirectories();

			Poco::FileOutputStream file(filePath);

			for (auto it = mAddressesIndices.begin(); it != mAddressesIndices.end(); it++) {
				auto inserted = sortedMap.insert(std::pair<int, std::string>(it->second, it->first));
				if (!inserted.second) {
					std::string currentPair = std::to_string(it->second) + ": " + it->first;
					std::string lastPair = std::to_string(inserted.first->first) + ": " + inserted.first->second;
					throw IndexAddressPairAlreadyExistException("AddressIndex::writeToFile", currentPair, lastPair);
				}
				file.write(it->first.data(), 32);
				file.write((const char*)&it->second, sizeof(int32_t));
			}
			auto hash = calculateHash(sortedMap);
			file.write((const char*)hash.data(), hash.size());
			file.close();
			fl->unlock(filePath);		
			mFileWritten = true;
		}

		std::unique_ptr<VirtualFile> AddressIndex::serialize()
		{
			if (checkFile()) {
				// current file seems containing all address indices 
				mFileWritten = true;
				return nullptr;
			}
			Poco::FastMutex::ScopedLock lock(mFastMutex);
			std::map<int, std::string> sortedMap;
			auto filePath = mFilePath.toString();
			
			auto vFile = std::make_unique<VirtualFile>(mAddressesIndices.size() * ( 32 + sizeof(int32_t) ) + crypto_generichash_BYTES);
			//Poco::FileOutputStream file(filePath);

			for (auto it = mAddressesIndices.begin(); it != mAddressesIndices.end(); it++) {
				auto inserted = sortedMap.insert(std::pair<int, std::string>(it->second, it->first));
				if (!inserted.second) {
					std::string currentPair = std::to_string(it->second) + ": " + it->first;
					std::string lastPair = std::to_string(inserted.first->first) + ": " + inserted.first->second;
					throw IndexAddressPairAlreadyExistException("AddressIndex::writeToFile", currentPair, lastPair);
				}
				vFile->write(it->first.data(), 32);
				vFile->write((const char*)&it->second, sizeof(int32_t));
			}
			auto hash = calculateHash(sortedMap);
			vFile->write((const char*)hash.data(), hash.size());
			return std::move(vFile);
		}

		std::string AddressIndex::getFileNameString()
		{
			return std::move(mFilePath.toString(Poco::Path::PATH_NATIVE));
		}


		bool AddressIndex::loadFromFile()
		{
			auto fl = FileLockManager::getInstance();
			int timeoutRounds = 100;
			auto filePath = mFilePath.toString();
			if (!fl->tryLockTimeout(filePath, timeoutRounds)) {
				return false;
			}
			Poco::File file(filePath);
			if (!file.exists()) {
				fl->unlock(filePath);
				return false;
			}

			auto fileSize = file.getSize();
			if (!fileSize) return false;

			auto readedSize = 0;
			Poco::FileInputStream fileStream(filePath);
			memory::Block hash(32);
			uint32_t index = 0;
			std::map<int, std::string> sortedMap;

			Poco::FastMutex::ScopedLock lock(mFastMutex);
			
			while (readedSize < fileSize) {
				fileStream.read((char*)hash.data(), 32);
				readedSize += 32;
				if (readedSize + sizeof(uint32_t) + 32 <= fileSize) {
					fileStream.read((char*)&index, sizeof(uint32_t));
					readedSize += sizeof(uint32_t);
					std::string hashString((const char*)hash.data(), hash.size());
					mAddressesIndices.insert(std::pair<std::string, int>(hashString, index));
					sortedMap.insert(std::pair<int, std::string>(index, hashString));
				}
				else {
					break;
				}
			}
			fl->unlock(filePath);
			auto compareHash = calculateHash(sortedMap);
			if (!hash.isTheSame(compareHash)) {
				throw HashMismatchException("address index file hash mismatch", hash, compareHash);
			}

			return true;
		}

		
		
	}

}