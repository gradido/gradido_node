#ifndef __GRADIDO_NODE_CONTROLLER_GROUP_INDEX_H
#define __GRADIDO_NODE_CONTROLLER_GROUP_INDEX_H

#include "../model/files/GroupIndex.h"
#include "../lib/DRHashList.h"

#include "Poco/Path.h"

#include "ControllerBase.h"
#include <unordered_map>

namespace controller {

	/*!
	 * @author Dario Rekowski
	 * 
	 * @date 2020-02-06
	 * 
	 * @brief storing group folder name in memory in hash list for fast access
	 *
	 * protected by FastMutex 
	 *
	 * TODO: adding function to adding group folder pair and save changed group.index file
	 */

	class GroupIndex : public ControllerBase
	{
	public:
		GroupIndex(model::files::GroupIndex* model);

		//! delete mModel
		~GroupIndex();

		//! \brief clear and refill hash list and doublet's vector from mModel
		//! protected with fast mutex
		//! \return hash map entry count
		size_t update();

		//! \brief get full folder path for group public key
		//! if the hash from groupPublicKey exist multiple time, getting folder from mModel, else from mHashList (faster)
		//! \return complete path to group folder or empty path if not group not found
		Poco::Path getFolder(const std::string& groupAlias);

		//! \brief collect all froup aliases from unordered map (not the fastest operation from unordered map)
		//! \return vector with group aliases registered to the node server
		std::vector<std::string> listGroupAliases();

	protected:

		struct GroupIndexEntry
		{
			std::string alias;
			std::string folderName;

			DHASH makeHash() {
				return DRMakeStringHash(alias.data(), alias.size());
			}
		};

		//! pointer to model get in constructor, deleted in deconstruction
		model::files::GroupIndex* mModel;

		std::unordered_map<std::string, GroupIndexEntry> mGroups;

		//! \brief clear hash list and doublet's vector
		void clear();

	};
};

#endif //__GRADIDO_NODE_CONTROLLER_GROUP_INDEX_H