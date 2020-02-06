#include "GroupIndex.h"

#include "Poco/Util/PropertyFileConfiguration.h"
#include "../../ServerGlobals.h"

#include "Poco/Path.h"


namespace model {
	namespace files {

		GroupIndex::GroupIndex(const std::string& _path)	
			: mConfig(new Poco::Util::LayeredConfiguration)
		{
			auto path = Poco::Path(ServerGlobals::g_FilesPath);
			path.append(_path);
			auto cfg = new Poco::Util::PropertyFileConfiguration(path.toString());
			mConfig->add(cfg);
		}
	}
}