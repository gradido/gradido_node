#include "GroupIndex.h"

#include "../ServerGlobals.h"

#include "Poco/File.h"

namespace controller {
	GroupIndex::GroupIndex(model::files::GroupIndex* model)
		: mModel(model)
	{

	}

	GroupIndex::~GroupIndex()
	{
		clear();
		delete mModel;
	}

	void GroupIndex::clear()
	{
		Poco::ScopedLock<Poco::Mutex> lock(mWorkingMutex);

	}

	size_t GroupIndex::update()
	{
		clear();
		Poco::ScopedLock<Poco::Mutex> lock(mWorkingMutex);
		auto cfg = mModel->getConfig();
		std::vector<std::string> keys;
		cfg->keys(keys);
		for (auto it = keys.begin(); it != keys.end(); it++) {
			GroupIndexEntry entry;
			entry.alias = *it;
			entry.folderName = cfg->getString(*it);

			mGroups.insert({entry.alias, entry});
		}
		return mGroups.size();
	}

	Poco::Path GroupIndex::getFolder(const std::string& groupAlias)
	{
		Poco::ScopedLock<Poco::Mutex> lock(mWorkingMutex);
		auto it = mGroups.find(groupAlias);
		if(it != mGroups.end()) {
			auto folder = Poco::Path(Poco::Path(ServerGlobals::g_FilesPath + '/'));
			folder.pushDirectory(it->second.folderName);
			Poco::File file(folder);
			if (!file.exists()) {
				file.createDirectory();
			}
			return folder;
		}

		return Poco::Path();
	}

	std::vector<std::string> GroupIndex::listGroupAliases()
	{
		std::vector<std::string> result;
		result.reserve(mGroups.size());
		for(auto it = mGroups.begin(); it != mGroups.end(); it++) {
			result.push_back(it->first);
		}
		return result;
	}
}
