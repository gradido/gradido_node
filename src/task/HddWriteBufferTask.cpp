#include "HddWriteBufferTask.h"
#include "../ServerGlobals.h"
#include "../SingletonManager/LoggerManager.h"

#include "Poco/File.h"

namespace task {
	HddWriteBufferTask::HddWriteBufferTask(std::unique_ptr<VirtualFile> vFile, Poco::Path path)
		: CPUTask(ServerGlobals::g_WriteFileCPUScheduler), mVirtualFile(std::move(vFile)), mFilePath(std::move(path))
	{
#ifdef _UNI_LIB_DEBUG
		setName(mFilePath.toString().data());
#endif
	}

	int HddWriteBufferTask::run()
	{
		Poco::Path pathDir = mFilePath;
		pathDir.makeDirectory().popDirectory();
		Poco::File fileFolder(pathDir);
		fileFolder.createDirectories();

		if (!mVirtualFile->writeToFile(mFilePath)) {
			LoggerManager::getInstance()->mErrorLogging.error("error locking file for writing: %s", mFilePath.toString());
		}
		return 0;
	}
}