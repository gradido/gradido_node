#include "HddWriteBufferTask.h"
#include "../ServerGlobals.h"
#include "../SingletonManager/LoggerManager.h"

#include "Poco/File.h"

namespace task {
	HddWriteBufferTask::HddWriteBufferTask(std::unique_ptr<VirtualFile> vFile, const Poco::Path& path)
		: CPUTask(ServerGlobals::g_WriteFileCPUScheduler), mVirtualFile(std::move(vFile)), mFileName(path.toString(Poco::Path::PATH_NATIVE))
	{
#ifdef _UNI_LIB_DEBUG
		setName(mFileName.data());
#endif
	}

	int HddWriteBufferTask::run()
	{
		Poco::File fileFolder(mFileName);
		fileFolder.createDirectories();

		if (!mVirtualFile->writeToFile(mFileName.data())) {
			LoggerManager::getInstance()->mErrorLogging.error("error locking file for writing: %s", mFileName);
		}
		return 0;
	}
}