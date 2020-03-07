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
		Poco::FastMutex::ScopedLock lock(mWorkingMutex);
		for (int i = 0; i < mHashList.getNItems(); i++) {
			delete mHashList.findByIndex(i);
		}
		mHashList.clear();
		mDoublettes.clear();
	}

	size_t GroupIndex::update()
	{
		clear();
		Poco::FastMutex::ScopedLock lock(mWorkingMutex);
		auto cfg = mModel->getConfig();
		std::vector<std::string> keys;
		cfg->keys(keys);
		for (auto it = keys.begin(); it != keys.end(); it++) {
			GroupIndexEntry* entry = new GroupIndexEntry;
			entry->hash = *it;
			entry->folderName = cfg->getString(*it);

			DHASH id = entry->makeHash();
			if (!mHashList.findByHash(id)) {
				mHashList.addByHash(id, entry);
			}
			else {
				delete entry;
				mDoublettes.push_back(id);
			}

		}
		return mHashList.getNItems();
	}

	Poco::Path GroupIndex::getFolder(const std::string& hash)
	{
		Poco::FastMutex::ScopedLock lock(mWorkingMutex);
		DHASH id = DRMakeStringHash(hash.data(), hash.size());
		std::string folderName;
		for (auto it = mDoublettes.begin(); it != mDoublettes.end(); it++) {
			if (id == *it) {
				auto cfg = mModel->getConfig();
				folderName = cfg->getString(hash);
			}
		}
		if (folderName.size() == 0) {
			GroupIndexEntry* entry = (GroupIndexEntry*)mHashList.findByHash(id);
			if (entry) {
				folderName = entry->folderName;
			}
		}
		if (folderName.size() > 0) {
			auto folder = Poco::Path(Poco::Path(ServerGlobals::g_FilesPath + '/'));
			folder.pushDirectory(folderName);
			Poco::File file(folder);
			if (!file.exists()) {
				file.createDirectory();
			}
			return folder;
		}
		return Poco::Path();
	}
}