#include "DeferredTransfer.h"
#include "../lib/LevelDBExceptions.h"
#include "../SingletonManager/CacheManager.h"
#include "../ServerGlobals.h"
#include "../task/DeferredTransferCacheCleanupTask.h"

#include <set>

namespace cache
{
	DeferredTransfer::DeferredTransfer(std::string_view groupFolder, std::string_view communityName)
		: mState(groupFolder), mTimerName(std::string(communityName).append("::deferredTransferCache")), mCommunityName(communityName)
	{

	}

	DeferredTransfer::~DeferredTransfer()
	{

	}

	bool DeferredTransfer::init()
	{
		std::lock_guard _lock(mFastMutex);
		if (!mState.init()) {
			return false;
		}
		loadFromState();
		CacheManager::getInstance()->getFuzzyTimer()->addTimer(mTimerName, this, ServerGlobals::g_CacheTimeout);
		return true;
	}

	void DeferredTransfer::exit()
	{
		std::lock_guard _lock(mFastMutex);
		CacheManager::getInstance()->getFuzzyTimer()->removeTimer(mTimerName);
		mState.exit();
	}

	void DeferredTransfer::reset()
	{
		std::lock_guard _lock(mFastMutex);
		mState.reset();
		mAddressIndexTransactionNrs.clear();
	}

	void DeferredTransfer::addTransactionNrForAddressIndex(uint32_t addressIndex, uint64_t transactionNr)
	{
		std::lock_guard _lock(mFastMutex);

		auto it = mAddressIndexTransactionNrs.find(addressIndex);
		if (it == mAddressIndexTransactionNrs.end()) {
			it = mAddressIndexTransactionNrs.insert({ addressIndex, {} }).first;
		}

		it->second.push_back(transactionNr);
		updateState(addressIndex, it->second);
	}

	std::vector<uint64_t> DeferredTransfer::getTransactionNrsForAddressIndex(uint32_t addressIndex)
	{
		std::lock_guard _lock(mFastMutex);
		auto it = mAddressIndexTransactionNrs.find(addressIndex);
		if (it != mAddressIndexTransactionNrs.end()) {
			return it->second;
		}
		return {};
	}

	bool DeferredTransfer::isDeferredTransfer(uint32_t addressIndex)
	{
		std::lock_guard _lock(mFastMutex);
		auto it = mAddressIndexTransactionNrs.find(addressIndex);
		if (it != mAddressIndexTransactionNrs.end()) {
			return true;
		}
		return false;
	}

	void DeferredTransfer::removeTransactionNrForAddressIndex(uint32_t addressIndex, uint64_t transactionNr)
	{
		std::lock_guard _lock(mFastMutex);
		auto it = mAddressIndexTransactionNrs.find(addressIndex);
		assert(it != mAddressIndexTransactionNrs.end());
		auto transactionNrs = it->second;
		for (auto transactionNrsIt = transactionNrs.begin(); transactionNrsIt != transactionNrs.end(); transactionNrsIt++) {
			if (*transactionNrsIt == transactionNr) {
				transactionNrsIt = transactionNrs.erase(transactionNrsIt);
				break;
			}
		}
		updateState(addressIndex, transactionNrs);
	}

	void DeferredTransfer::removeAddressIndex(uint32_t addressIndex)
	{
		std::lock_guard _lock(mFastMutex);
		mAddressIndexTransactionNrs.erase(addressIndex);
		mState.removeState(std::to_string(addressIndex).data());
	}

	void DeferredTransfer::removeAddressIndexes(std::function<bool(uint32_t addressIndex, uint64_t transactionNr)> canBeRemoved) {
		std::lock_guard _lock(mFastMutex);
		for (auto it = mAddressIndexTransactionNrs.begin(); it != mAddressIndexTransactionNrs.end(); it++) {
			if (canBeRemoved(it->first, it->second.front())) {
				it = mAddressIndexTransactionNrs.erase(it);
				mState.removeState(std::to_string(it->first).data());
				if (it == mAddressIndexTransactionNrs.end()) {
					return;
				}
			}
		}
	}


	TimerReturn DeferredTransfer::callFromTimer()
	{
		auto cleanupTask = std::make_shared<task::DeferredTransferCacheCleanupTask>(mCommunityName);
		cleanupTask->scheduleTask(cleanupTask);
		return TimerReturn::GO_ON;
	}

	void DeferredTransfer::updateState(uint32_t addressIndex, std::vector<uint64_t> transactionNrs)
	{
		mState.updateState(std::to_string(addressIndex).data(), *transactionNrsToString(transactionNrs).get());
	}

	void DeferredTransfer::loadFromState()
	{
		mAddressIndexTransactionNrs.clear();
		mState.readAllStates([&](const std::string key, const std::string value) {
			auto transactionNrs = transactionNrsToVector(value);
			mAddressIndexTransactionNrs.insert({ std::stoul(key), transactionNrs });
		});
	}

	std::unique_ptr<std::string> DeferredTransfer::transactionNrsToString(std::vector<uint64_t> transactionNrs)
	{
		std::unique_ptr<std::string> transactionNrsString = std::make_unique<std::string>();
		std::set<uint64_t> uniqueTransactionNrs;
		for (auto it = transactionNrs.begin(); it != transactionNrs.end(); it++) {
			// if transaction nr already exist, skip
			if (!uniqueTransactionNrs.insert(*it).second) {
				continue;
			}
			if (!transactionNrsString->empty()) {
				transactionNrsString->append(",");
			}
			transactionNrsString->append(std::to_string(*it));
		}
		return std::move(transactionNrsString);
	}

	std::vector<uint64_t> DeferredTransfer::transactionNrsToVector(const std::string& transactionNrsString)
	{
		std::vector<uint64_t> transactionNrs;
		// taken code from https://stackoverflow.com/questions/5167625/splitting-a-c-stdstring-using-tokens-e-g
		std::string::size_type prev_pos = 0, pos = 0;

		while ((pos = transactionNrsString.find(',', pos)) != std::string::npos)
		{
			transactionNrs.push_back(std::stoull(transactionNrsString.substr(prev_pos, pos - prev_pos)));
			prev_pos = ++pos;
		}

		transactionNrs.push_back(std::stoull(transactionNrsString.substr(prev_pos, pos - prev_pos))); // Last word
		return transactionNrs;
	}
}