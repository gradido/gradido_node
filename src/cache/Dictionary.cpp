#include "Dictionary.h"
#include "../ServerGlobals.h"
#include "../SingletonManager/CacheManager.h"
#include "../SystemExceptions.h"
#include "../model/files/FileExceptions.h"

#include "loguru/loguru.hpp"

namespace cache {
	
	Dictionary::Dictionary(std::string_view fileName)
		: mFileName(fileName),
		mLastIndex(0),
		mDictionaryFile(std::make_shared<model::files::Dictionary>(fileName))
	{
	}

	Dictionary::~Dictionary()
	{
	}

	bool Dictionary::init()
	{
		std::lock_guard _lock(mFastMutex);
		if (!mDictionaryFile->init()) {
			return false;
		}
		mLastIndex = mDictionaryFile->getIndexCount();
		CacheManager::getInstance()->getFuzzyTimer()->addTimer(mFileName, this, ServerGlobals::g_TimeoutCheck, -1);
		return true;
	}

	void Dictionary::exit()
	{
		std::lock_guard _lock(mFastMutex);
		auto result = CacheManager::getInstance()->getFuzzyTimer()->removeTimer(mFileName);
		if (result != 1 && result != -1) {
			LOG_F(ERROR, "error removing timer");
		}
		mDictionaryFile->exit();
	}

	void Dictionary::reset()
	{
		std::lock_guard _lock(mFastMutex);
		mDictionaryFile->reset();
		mLastIndex = 0;
	}

	bool Dictionary::addStringIndex(const std::string& string, uint32_t index)
	{
		std::lock_guard _lock(mFastMutex);
		mLastIndex = index;

		return mDictionaryFile->add(string, index);
	}

	uint32_t Dictionary::getIndexForString(const std::string& string)  const
	{
		std::lock_guard _lock(mFastMutex);
		return mDictionaryFile->getIndexForString(string);
	}

	const std::string& Dictionary::getStringForIndex(uint32_t index) const
	{
		std::lock_guard _lock(mFastMutex);
		return mDictionaryFile->getStringForIndex(index);
	}

	uint32_t Dictionary::getOrAddIndexForString(const std::string& string)
	{
		std::lock_guard _lock(mFastMutex);
		auto index = mDictionaryFile->getIndexForString(string);
		if (!index) {
			index = ++mLastIndex;
			mDictionaryFile->add(string, index);
		}
		return index;
	}

	TimerReturn Dictionary::callFromTimer()
	{
		Poco::ScopedLock _lock(mFastMutex);

		if (Timepoint() - mWaitingForNextFileWrite > ServerGlobals::g_WriteToDiskTimeout
			) {
			mWaitingForNextFileWrite = Timepoint();
			if (!mDictionaryFile->isFileWritten()) {
				task::TaskPtr serializeAndWriteToFileTask = std::make_shared<task::SerializeToVFileTask>(mDictionaryFile);
				serializeAndWriteToFileTask->setFinishCommand(new model::files::SuccessfullWrittenToFileCommand(mDictionaryFile));
				serializeAndWriteToFileTask->scheduleTask(serializeAndWriteToFileTask);
			}
		}
		return TimerReturn::GO_ON;
	}
}
