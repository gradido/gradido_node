#ifndef __GRADIDO_NODE_CONTROLLER_GROUP_INDEX_H
#define __GRADIDO_NODE_CONTROLLER_GROUP_INDEX_H

#include "../model/files/GroupIndex.h"
#include "../lib/DRHashList.h"

#include "Poco/Path.h"

#include "ControllerBase.h"

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
		Poco::Path getFolder(const std::string& groupPublicKey);

	protected:

		struct GroupIndexEntry {
			std::string hash;
			std::string folderName;

			DHASH makeHash() {
				return DRMakeStringHash(hash.data(), hash.size());
			}
		};

		//! pointer to model get in constructor, deleted in deconstruction
		model::files::GroupIndex* mModel;
		DRHashList mHashList;
		//! containing group public key hash doublet's
		std::vector<DHASH> mDoublettes;

		//! \brief clear hash list and doublet's vector
		void clear();

	};
};

#endif //__GRADIDO_NODE_CONTROLLER_GROUP_INDEX_H