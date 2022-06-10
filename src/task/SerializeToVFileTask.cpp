#include "SerializeToVFileTask.h"
#include "../ServerGlobals.h"

#include "HddWriteBufferTask.h"

namespace task {

	SerializeToVFileTask::SerializeToVFileTask(Poco::SharedPtr<ISerializeToVFile> dataProvider)
		: CPUTask(ServerGlobals::g_CPUScheduler), mDataProvider(dataProvider)
	{
#ifdef _UNI_LIB_DEBUG
		auto fileNameString = dataProvider->getFileNameString();
		setName(fileNameString.data());
#endif
	}

	SerializeToVFileTask::~SerializeToVFileTask()
	{

	}

	int SerializeToVFileTask::run()
	{
		auto vfile = mDataProvider->serialize();
		if (!vfile) return 0;
		TaskPtr hddWriteBufferTask = new HddWriteBufferTask(std::move(vfile), std::move(mDataProvider->getFileNameString()));
		if (mFinishCommand) {
			hddWriteBufferTask->setFinishCommand(mFinishCommand);
			mFinishCommand = nullptr;
		}
		hddWriteBufferTask->scheduleTask(hddWriteBufferTask);		
		return 0;
	}
}