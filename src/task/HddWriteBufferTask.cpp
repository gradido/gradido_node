#include "HddWriteBufferTask.h"
#include "../ServerGlobals.h"
#include "gradido_blockchain/Application.h"

#include "loguru/loguru.hpp"

#include <filesystem>

namespace task {
	HddWriteBufferTask::HddWriteBufferTask(std::unique_ptr<VirtualFile> vFile, const std::string& path)
		: CPUTask(ServerGlobals::g_WriteFileCPUScheduler), mVirtualFile(std::move(vFile)), mFilePath(path)
	{
#ifdef _UNI_LIB_DEBUG
		setName(mFilePath.data());
#endif
	}

	int HddWriteBufferTask::run()
	{
		try {
			std::filesystem::path pathDir(mFilePath);
			pathDir.remove_filename();
			if (!std::filesystem::exists(pathDir)) {
				std::filesystem::create_directories(pathDir);
			}
			mVirtualFile->writeToFile(mFilePath.data());
		}
		catch (GradidoBlockchainException& ex) {
			LOG_F(FATAL, "error writing into file: %s, gradido blockchain exception: %s", mFilePath.data(), ex.getFullString().data());
			Application::terminate();
		}
		catch (std::exception& e) {
			LOG_F(FATAL, "error writing into file: %s, exception: %s", mFilePath.data(), e.what());
			Application::terminate();
		}
		return 0;
	}
}