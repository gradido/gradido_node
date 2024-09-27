#include "AddressIndex.h"
#include "sodium.h"
#include "../ServerGlobals.h"
#include "../SingletonManager/CacheManager.h"
#include "../blockchain/FileBased.h"
#include "../model/files/FileExceptions.h"

#include "loguru/loguru.hpp"

namespace cache {
	
	AddressIndex::AddressIndex(std::string_view path, gradido::blockchain::FileBased* parent)
		: mGroupPath(path),
		mLastIndex(0),
		mAddressIndexFile(std::make_shared<model::files::AddressIndex>(mGroupPath)),
		mParent(parent)
	{
	}

	AddressIndex::~AddressIndex()
	{
	}

	bool AddressIndex::init()
	{
		std::lock_guard _lock(mFastMutex);
		if (!mAddressIndexFile->init()) {
			return false;
		}
		mLastIndex = mAddressIndexFile->getIndexCount();
		CacheManager::getInstance()->getFuzzyTimer()->addTimer(mGroupPath, this, ServerGlobals::g_TimeoutCheck, -1);
		return true;
	}

	void AddressIndex::exit()
	{
		std::lock_guard _lock(mFastMutex);
		auto result = CacheManager::getInstance()->getFuzzyTimer()->removeTimer(mGroupPath);
		if (result != 1 && result != -1) {
			LOG_F(ERROR, "error removing timer");
		}
		mAddressIndexFile->exit();
	}

	void AddressIndex::reset()
	{
		std::lock_guard _lock(mFastMutex);
		mAddressIndexFile.reset();
		mLastIndex = 0;
	}

	bool AddressIndex::addAddressIndex(const std::string& address, uint32_t index)
	{
		std::lock_guard _lock(mFastMutex);
		mLastIndex = index;

		return mAddressIndexFile->add(address, index);
	}

	uint32_t AddressIndex::getIndexForAddress(const std::string& address)
	{
		return mAddressIndexFile->getIndexForAddress(address);
	}


	uint32_t AddressIndex::getOrAddIndexForAddress(const std::string& address)
	{
		std::lock_guard _lock(mFastMutex);
		auto index = mAddressIndexFile->getIndexForAddress(address);
		if (!index) {
			index = ++mLastIndex;
			mAddressIndexFile->add(address, index);
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


	std::string AddressIndex::getAddressIndexFilePathForAddress(const std::string& address)
	{
		return "pubkeys.index";
	}

	TimerReturn AddressIndex::callFromTimer()
	{
		Poco::ScopedLock _lock(mFastMutex);

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
