#include "AddressIndex.h"
#include "FileExceptions.h"
#include "sodium.h"

#include "../../SingletonManager/FileLockManager.h"
#include "../../ServerGlobals.h"

#include "loguru/loguru.hpp"

#include <fstream>
#include <filesystem>
#include <map>

namespace model {
	namespace files {

		AddressIndex::AddressIndex(std::string_view fileName)
			: mFileName(fileName), mFileWritten(false)
		{
			
		}

		AddressIndex::~AddressIndex()
		{
			
		}

		bool AddressIndex::init()
		{
			try {
				if (loadFromFile()) {
					mFileWritten = true;
				}
			}
			catch (HashMismatchException& ex) {
				LOG_F(ERROR, "%s file has invalid hash: %s", mFileName.data(), ex.getFullString().data());
				return false;
			}
			return true;
		}

		void AddressIndex::exit()
		{
			// will return nullptr if file is up to date
			auto vFile = serialize();
			if (vFile) {
				vFile->writeToFile(mFileName);
				mFileWritten = true;
			}
		}

		void AddressIndex::reset()
		{
			auto fl = FileLockManager::getInstance();
			int timeoutRounds = 100;
			if (!fl->tryLockTimeout(mFileName, timeoutRounds)) {
				throw LockException("cannot lock file for reset", mFileName);
			}
			std::filesystem::remove(mFileName);
			mAddressesIndices.clear();
			// both is empty so technically addess indices are written to file
			mFileWritten = true;
			fl->unlock(mFileName);
		}

		bool AddressIndex::add(const std::string& address, uint32_t index)
		{
			if (address == "") {
				throw GradidoNodeInvalidDataException("empty address");
			}
			mFileWritten = false;
			mFastMutex.lock();
			bool result = mAddressesIndices.insert(std::pair<std::string, uint32_t>(address, index)).second;
			mFastMutex.unlock();

			return result;
		}

		uint32_t AddressIndex::getIndexForAddress(const std::string &address)
		{
			std::lock_guard _lock(mFastMutex);

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
			if (!fl->tryLockTimeout(mFileName, timeoutRounds)) {
				return false;
			}
			// ate = output position starts at end of file
			std::ifstream file(mFileName, std::ifstream::binary | std::ifstream::ate);
			if (file.is_open() && file.good()) {
				if (file.tellg() == calculateFileSize()) {
					fl->unlock(mFileName);
					return true;
				}
			}
			fl->unlock(mFileName);
			return false;
		}

		void AddressIndex::setFileWritten()
		{
			std::lock_guard _lock(mFastMutex);
			mFileWritten = true;
		}

		size_t AddressIndex::calculateFileSize()
		{
			std::lock_guard _lock(mFastMutex);

			auto mapEntryCount = mAddressesIndices.size();
			return mapEntryCount * (sizeof(uint32_t) + 32) + 32;
		}

		MemoryBin AddressIndex::calculateHash(const std::map<uint32_t, std::string>& sortedMap)
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

		std::unique_ptr<VirtualFile> AddressIndex::serialize()
		{
			if (checkFile()) {
				// current file seems containing all address indices 
				mFileWritten = true;
				return nullptr;
			}
			std::lock_guard _lock(mFastMutex);
			std::map<uint32_t, std::string> sortedMap;
			
			auto vFile = std::make_unique<VirtualFile>(mAddressesIndices.size() * ( 32 + sizeof(uint32_t) ) + crypto_generichash_BYTES);
			//Poco::FileOutputStream file(filePath);

			for (auto it = mAddressesIndices.begin(); it != mAddressesIndices.end(); it++) {
				auto inserted = sortedMap.insert({it->second, it->first});
				if (!inserted.second) {
					std::string currentPair = std::to_string(it->second) + ": " + it->first;
					std::string lastPair = std::to_string(inserted.first->first) + ": " + inserted.first->second;
					throw IndexAddressPairAlreadyExistException("AddressIndex::writeToFile", currentPair, lastPair);
				}
				vFile->write(it->first.data(), 32);
				vFile->write((const char*)&it->second, sizeof(uint32_t));
			}
			auto hash = calculateHash(sortedMap);
			vFile->write((const char*)hash.data(), hash.size());
			return std::move(vFile);
		}

		bool AddressIndex::loadFromFile()
		{
			auto fl = FileLockManager::getInstance();
			int timeoutRounds = 100;
			if (!fl->tryLockTimeout(mFileName, timeoutRounds)) {
				return false;
			}
			std::ifstream file(mFileName, std::ifstream::binary | std::ifstream::ate);
			if (!file.is_open() || !file.good()) {
				fl->unlock(mFileName);
				return false;
			}

			auto fileSize = file.tellg();
			if (!fileSize) return false;

			auto readedSize = 0;
			memory::Block hash(32);
			uint32_t index = 0;
			std::map<uint32_t, std::string> sortedMap;
			file.seekg(0, file.beg);

			std::lock_guard _lock(mFastMutex);
			
			while (readedSize < fileSize) {
				file.read((char*)hash.data(), 32);
				readedSize += 32;
				if (readedSize + sizeof(uint32_t) + 32 <= fileSize) {
					file.read((char*)&index, sizeof(uint32_t));
					readedSize += sizeof(uint32_t);
					std::string hashString((const char*)hash.data(), hash.size());
					mAddressesIndices.insert({ hashString, index });
					sortedMap.insert({ index, hashString });
				}
				else {
					break;
				}
			}
			fl->unlock(mFileName);
			auto compareHash = calculateHash(sortedMap);
			if (!hash.isTheSame(compareHash)) {
				throw HashMismatchException("address index file hash mismatch", hash, compareHash);
			}

			return true;
		}

		
		
	}

}