#include "AddressIndex.h"
#include "sodium.h"
#include "../ServerGlobals.h"
#include "../SingletonManager/LoggerManager.h"
#include "../SingletonManager/CacheManager.h"
#include "Group.h"
#include "../model/files/FileExceptions.h"

namespace controller {
	AddressIndex::AddressIndex(Poco::Path path, uint32_t lastIndex, Group* parent)
		: mGroupPath(path), mLastIndex(lastIndex), mAddressIndicesCache(ServerGlobals::g_CacheTimeout), mParent(parent)
	{
		Poco::Path addressIndexPath(mGroupPath);
		addressIndexPath.append(getAddressIndexFilePathForAddress("HalloWelt"));
		try {
			mAddressIndexFile = new model::files::AddressIndex(addressIndexPath);
		}
		catch (model::files::HashMismatchException& ex) {
			LoggerManager::getInstance()->mErrorLogging.critical(ex.getFullString());
			mParent->resetAllIndices();
			Poco::SharedPtr<model::files::AddressIndex> newAddressIndex(new model::files::AddressIndex(addressIndexPath));
		}		
		CacheManager::getInstance()->getFuzzyTimer()->addTimer(addressIndexPath.toString(), this, ServerGlobals::g_TimeoutCheck, -1);
	}

	AddressIndex::~AddressIndex()
	{
		Poco::Path addressIndexPath(mGroupPath);
		addressIndexPath.append(getAddressIndexFilePathForAddress("HalloWelt"));
		if (CacheManager::getInstance()->getFuzzyTimer()->removeTimer(addressIndexPath.toString()) != 1) {
			printf("[controller::~AddressIndex]] error removing timer\n");
		}
	}

	bool AddressIndex::addAddressIndex(const std::string& address, uint32_t index)
	{
		auto addressIndex = getAddressIndex(address);
		if (addressIndex.isNull()) {
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
		assert(!addressIndex.isNull());
		auto index = addressIndex->getIndexForAddress(address);
		if (!index) {
			index = ++mLastIndex;
			addressIndex->add(address, index);
		}
		return index;
	}

	std::vector<uint32_t> AddressIndex::getOrAddIndicesForAddresses(std::vector<MemoryBin*>& publicKeys, bool clearMemoryBin/* = false*/)
	{
		auto mm = MemoryManager::getInstance();
		std::vector<uint32_t> results;
		results.reserve(publicKeys.size());
		for (auto it = publicKeys.begin(); it != publicKeys.end(); it++) {
			results.push_back(getOrAddIndexForAddress(*(*it)->copyAsString().get()));
			if (clearMemoryBin) {
				mm->releaseMemory(*it);
			}
		}
		if (clearMemoryBin) {
			publicKeys.clear();
		}
		return results;
	}

	Poco::SharedPtr<model::files::AddressIndex> AddressIndex::getAddressIndex(const std::string& address)
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

	Poco::Path AddressIndex::getAddressIndexFilePathForAddress(const std::string& address)
	{
		Poco::Path addressIndexPath;
		char firstBytesHex[3]; memset(firstBytesHex, 0, 3);
		sodium_bin2hex(firstBytesHex, 3, (const unsigned char*)address.data(), 1);
		//printf("bytes %x\n", firstBytes);
		std::string firstBytesString = firstBytesHex;
		addressIndexPath.pushDirectory("pubkeys");
		//addressIndexPath.pushDirectory("_" + firstBytesString.substr(0, 2));
		std::string file = "_" + firstBytesString.substr(0, 2);
		file += ".index";
		//addressIndexPath.append(file);
		addressIndexPath.append("pubkeys.index");

		return addressIndexPath;
	}

	TimerReturn AddressIndex::callFromTimer()
	{
		Poco::ScopedLock _lock(mWorkingMutex);

		if (Poco::Timespan(Poco::DateTime() - mWaitingForNextFileWrite).totalSeconds() > ServerGlobals::g_WriteToDiskTimeout) {
			mWaitingForNextFileWrite = Poco::DateTime();
			if (!mAddressIndexFile->isFileWritten()) {
				task::TaskPtr serializeAndWriteToFileTask = new task::SerializeToVFileTask(mAddressIndexFile);
				serializeAndWriteToFileTask->setFinishCommand(new model::files::SuccessfullWrittenToFileCommand(mAddressIndexFile));
				serializeAndWriteToFileTask->scheduleTask(serializeAndWriteToFileTask);
			}			
		}
		return TimerReturn::GO_ON;
	}

}