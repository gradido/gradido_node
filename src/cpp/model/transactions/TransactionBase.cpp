#include "TransactionBase.h"
#include "../../SingletonManager/GroupManager.h"
#include "../../controller/Group.h"

namespace model {

	TransactionBase::TransactionBase()
		: mReferenceCount(1), mParent(nullptr)
	{

	}

	TransactionBase::TransactionBase(Transaction* parent)
		: mReferenceCount(1), mParent(parent)
	{

	}

	void TransactionBase::duplicate()
	{
		Poco::Mutex::ScopedLock lock(mAutoPtrMutex);
		mReferenceCount++;
	}

	void TransactionBase::setParent(Transaction* parent)
	{
		mParent = parent;
	}

	void TransactionBase::release()
	{
		Poco::Mutex::ScopedLock lock(mAutoPtrMutex);
		mReferenceCount--;
		//printf("[Task::release] new value: %d\n", mReferenceCount);
		if (0 == mReferenceCount) {
			//mReferenceMutex.unlock();
			delete this;
			return;
		}
	}

	void TransactionBase::addBase58GroupHashes(TransactionBase* parent)
	{
		for (auto it = parent->mBase58GroupHashes.begin(); it != parent->mBase58GroupHashes.end(); it++) {
			mBase58GroupHashes.push_back(*it);
		}

	}

	void TransactionBase::collectGroups(std::vector<std::string> groupBase58Hashes, std::list<controller::Group*>& container)
	{
		auto gm = GroupManager::getInstance();
		for (auto it = groupBase58Hashes.begin(); it != groupBase58Hashes.end(); it++) {
			auto group = gm->findGroup(*it);
			if (nullptr == group) {
				addError(new ParamError(__FUNCTION__, "group not found", (*it).data()));
			}
			else {
				container.push_back(group);
			}
		}
	}

	std::list<controller::Group*> TransactionBase::getGroups()
	{
		
		std::list<controller::Group*> groups;

		if (mBase58GroupHashes.size() > 0) {
			collectGroups(mBase58GroupHashes, groups);
		}
		if (mParent && mParent->mBase58GroupHashes.size() > 0) {
			collectGroups(mParent->mBase58GroupHashes, groups);
		}

		// make unique
		groups.sort();
		controller::Group* lastGroup = nullptr;
		for (auto it = groups.begin(); it != groups.end(); it++) {
			if (*it == lastGroup) {
				it = groups.erase(it);
			}
			else {
				lastGroup = *it;
			}
		}
		return groups;
	}
}