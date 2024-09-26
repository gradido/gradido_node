#include "AddressIndex.h"
#include "sodium.h"
#include "../ServerGlobals.h"
#include "../SingletonManager/CacheManager.h"
#include "../blockchain/FileBased.h"
#include "../model/files/FileExceptions.h"

#include "loguru/loguru.hpp"

namespace controller {
	AddressIndex::AddressIndex(std::string_view path, uint32_t lastIndex, gradido::blockchain::FileBased* parent)
		: mGroupPath(path), mLastIndex(lastIndex), 
		  mAddressIndicesCache(ServerGlobals::g_CacheTimeout), 
		  mParent(parent)
	{
		Poco::Path addressIndexPath(mGroupPath);
		addressIndexPath.append(getAddressIndexFilePathForAddress("HalloWelt"));
		try {
			mAddressIndexFile = std::make_shared<model::files::AddressIndex>(addressIndexPath);
		}
		catch (model::files::HashMismatchException& ex) {
			LOG_F(FATAL, ex.getFullString().data());
			mParent->resetAllIndices();
			Poco::SharedPtr<model::files::AddressIndex> newAddressIndex(new model::files::AddressIndex(addressIndexPath));
		}
		CacheManager::getInstance()->getFuzzyTimer()->addTimer(addressIndexPath.toString(), this, ServerGlobals::g_TimeoutCheck, -1);
	}

	AddressIndex::~AddressIndex()
	{
		Poco::Path addressIndexPath(mGroupPath);
		addressIndexPath.append(getAddressIndexFilePathForAddress("HalloWelt"));
		auto result = CacheManager::getInstance()->getFuzzyTimer()->removeTimer(addressIndexPath.toString());
		if (result != 1 && result != -1) {
			LOG_F(ERROR, "error removing timer");
		}
	}

	bool AddressIndex::addAddressIndex(const std::string& address, uint32_t index)
	{
		auto addressIndex = getAddressIndex(address);
		if (!addressIndex) {
			return false;
		}
		mWorkingMutex.lock();
		mLastIndex = index;
		mWorkingMutex.unlock();

		return addressIndex->add(address, index);
	}

	uint32_t AddressIndex::getIndexForAddress(const std::string &address)
	{
		return getAddressIndex(address)->getIndexForAddress(address);
	}


	uint32_t AddressIndex::getOrAddIndexForAddress(const std::string& address)
	{
		auto addressIndex = getAddressIndex(address);
		assert(addressIndex);
		auto index = addressIndex->getIndexForAddress(address);
		if (!index) {
			index = ++mLastIndex;
			addressIndex->add(address, index);
		}
		return index;
	}

	std::vector<uint32_t> AddressIndex::getOrAddIndicesForAddresses(const std::vector<memory::ConstBlockPtr>& publicKeys)
	{
		std::vector<uint32_t> results;
		results.reserve(publicKeys.size());
		for (auto it = publicKeys.begin(); it != publicKeys.end(); it++) {
			results.push_back(getOrAddIndexForAddress((*it)->copyAsString()));
		}
		return results;
	}

	std::shared_ptr<model::files::AddressIndex> AddressIndex::getAddressIndex(const std::string& address)
	{
		return mAddressIndexFile;
		/*uint8_t firstBytes = 0;
		memcpy(&firstBytes, address.data(), sizeof(uint8_t));
		auto entry = mAddressIndicesCache.get(firstBytes);

		if (entry.isNull()) {

			Poco::Path addressIndexPath(mGroupPath);
			addressIndexPath.append(getAddressIndexFilePathForAddress(address));
			try {
				Poco::SharedPtr<model::files::AddressIndex> newAddressIndex(new model::files::AddressIndex(addressIndexPath));
				mAddressIndicesCache.add(firstBytes, newAddressIndex);
				return newAddressIndex;
			}
			catch (model::files::HashMismatchException& ex) {
				LoggerManager::getInstance()->mErrorLogging.critical(ex.getFullString());
				mParent->resetAllIndices();
				Poco::SharedPtr<model::files::AddressIndex> newAddressIndex(new model::files::AddressIndex(addressIndexPath));
				mAddressIndicesCache.add(firstBytes, newAddressIndex);
				return newAddressIndex;
			}
		}

		return entry;*/
	}

	std::string AddressIndex::getAddressIndexFilePathForAddress(const std::string& address)
	{
		return "pubkeys.index";

		// use this code if one file is not longer enough
		/* 
		std::stringstream addressIndexPath;
		
		char firstBytesHex[3]; memset(firstBytesHex, 0, 3);
		sodium_bin2hex(firstBytesHex, 3, (const unsigned char*)address.data(), 1);
		addressIndexPath << "pubkeys/_" << firstBytesHex << ".index";

		return addressIndexPath.str();
		*/
	}

	TimerReturn AddressIndex::callFromTimer()
	{
		Poco::ScopedLock _lock(mWorkingMutex);

		if (Timepoint() - mWaitingForNextFileWrite > ServerGlobals::g_WriteToDiskTimeout
		) {
			mWaitingForNextFileWrite = Timepoint();
			if (!mAddressIndexFile->isFileWritten()) {
				task::TaskPtr serializeAndWriteToFileTask = new task::SerializeToVFileTask(mAddressIndexFile);
				serializeAndWriteToFileTask->setFinishCommand(new model::files::SuccessfullWrittenToFileCommand(mAddressIndexFile));
				serializeAndWriteToFileTask->scheduleTask(serializeAndWriteToFileTask);
			}
		}
		return TimerReturn::GO_ON;
	}

}
