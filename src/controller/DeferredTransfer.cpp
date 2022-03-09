#include "DeferredTransfer.h"
#include "../lib/LevelDBExceptions.h"

namespace controller
{
	DeferredTransfer::DeferredTransfer(const Poco::Path& groupFolder)
		: mState(Poco::Path(groupFolder, Poco::Path("deferredTransferCache")))
	{
		loadFromState();
	}

	DeferredTransfer::~DeferredTransfer()
	{

	}

	void DeferredTransfer::addTransactionNrForAddressIndex(uint32_t addressIndex, uint64_t transactionNr)
	{
		Poco::ScopedLock<Poco::Mutex> _lock(mWorkingMutex);

		auto it = mAddressIndexTransactionNrs.find(addressIndex);
		if (it == mAddressIndexTransactionNrs.end()) {
			it = mAddressIndexTransactionNrs.insert({ addressIndex, {} }).first;
		}
		
		it->second.push_back(transactionNr);
		updateState(addressIndex, it->second);		
	}

	std::vector<uint64_t> DeferredTransfer::getTransactionNrsForAddressIndex(uint32_t addressIndex)
	{
		Poco::ScopedLock<Poco::Mutex> _lock(mWorkingMutex);
		auto it = mAddressIndexTransactionNrs.find(addressIndex);
		if (it != mAddressIndexTransactionNrs.end()) {
			return it->second;
		}
		return {};
	}

	void DeferredTransfer::removeTransactionNrForAddressIndex(uint32_t addressIndex, uint64_t transactionNr)
	{
		Poco::ScopedLock<Poco::Mutex> _lock(mWorkingMutex);

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

	void DeferredTransfer::updateState(uint32_t addressIndex, std::vector<uint64_t> transactionNrs)
	{
		Poco::ScopedLock<Poco::Mutex> _lock(mWorkingMutex);
		mState.setKeyValue(std::to_string(addressIndex), *transactionNrsToString(transactionNrs).get());
	}

	void DeferredTransfer::loadFromState()
	{
		Poco::ScopedLock<Poco::Mutex> _lock(mWorkingMutex);
		auto it = mState.getIterator();
		assert(it);
		mAddressIndexTransactionNrs.clear();
		for (it->SeekToFirst(); it->Valid(); it->Next()) {
			auto transactionNrs = transactionNrsToVector(it->value().ToString());
			mAddressIndexTransactionNrs.insert({ std::stoul(it->key().data()), transactionNrs });
		}
		if (!it->status().ok()) {
			throw LevelDBStatusException("error in iterate over active deferred transfers in leveldb", it->status());
		}
		delete it;
	}

	std::unique_ptr<std::string> DeferredTransfer::transactionNrsToString(std::vector<uint64_t> transactionNrs)
	{
		std::unique_ptr<std::string> transactionNrsString = std::make_unique<std::string>();
		for (auto it = transactionNrs.begin(); it != transactionNrs.end(); it++) {
			if (it != transactionNrs.begin()) {
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