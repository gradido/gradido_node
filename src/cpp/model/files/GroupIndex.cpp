#include "GroupIndex.h"

#include "Poco/Util/PropertyFileConfiguration.h"
#include "../../ServerGlobals.h"
#include "../../SingletonManager/FileLockManager.h"

#include "Poco/Path.h"
#include "Poco/Thread.h"

namespace model {
	namespace files {

		GroupIndex::GroupIndex(const std::string& _path)	
			: mConfig(new Poco::Util::LayeredConfiguration)
		{
			auto fl = FileLockManager::getInstance();
			auto path = Poco::Path(ServerGlobals::g_FilesPath);
			path.append(_path);
			auto pathString = path.toString();
			while (!fl->tryLock(pathString)) {
				Poco::Thread::sleep(100);
			}
			auto cfg = new Poco::Util::PropertyFileConfiguration(pathString);
			mConfig->add(cfg);
			fl->unlock(pathString);
		}
	}
}