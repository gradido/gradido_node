#include "TransactionBase.h"
#include "../../SingletonManager/GroupManager.h"
#include "../../controller/Group.h"

namespace model {

	TransactionBase::TransactionBase()
		: mReferenceCount(1), mParent(nullptr)
	{

	}

	TransactionBase::TransactionBase(controller::Group* parent)
		: mReferenceCount(1), mParent(parent)
	{

	}

	void TransactionBase::duplicate()
	{
		Poco::Mutex::ScopedLock lock(mAutoPtrMutex);
		mReferenceCount++;
	}

	void TransactionBase::setParent(Poco::SharedPtr<controller::Group> parent)
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

	void TransactionBase::addGroupAliases(TransactionBase* parent)
	{
		for (auto it = parent->mGroupAliases.begin(); it != parent->mGroupAliases.end(); it++) {
			mGroupAliases.push_back(*it);
		}

	}

	void TransactionBase::collectGroups(std::vector<std::string> groupAliases, std::list<controller::Group*>& container)
	{
		auto gm = GroupManager::getInstance();
		for (auto it = groupAliases.begin(); it != groupAliases.end(); it++) {
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

		if (mGroupAliases.size() > 0) {
			collectGroups(mGroupAliases, groups);
		}
		if (mParent && mParent->mGroupAliases.size() > 0) {
			collectGroups(mParent->mGroupAliases, groups);
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
