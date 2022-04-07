#ifndef __GRADIDO_NODE_CONTROLLER_GROUP_REGISTER_GROUP_H
#define __GRADIDO_NODE_CONTROLLER_GROUP_REGISTER_GROUP_H

#include "Group.h"

namespace controller {

	/*!
		@brief struct for holding two important informations for every group
	*/

	struct GroupEntry
	{
		GroupEntry(uint64_t _transactionNr, uint32_t _coinColor)
			: transactionNr(_transactionNr), coinColor(_coinColor)
		{}
		uint64_t transactionNr;
		uint32_t coinColor;
	};

	/*!
		@author einhornimmond

		@date 27.02.2022

		@brief Special Group on which all Gradido Node Server listen, contain group registries
	*/

	class GroupRegisterGroup: public Group
	{
	public:
		GroupRegisterGroup();
		~GroupRegisterGroup();

		GroupEntry findGroup(const std::string& groupAlias);
		uint32_t generateUniqueCoinColor();
		bool isCoinColorUnique(uint32_t coinColor);
		bool isGroupAliasUnique(const std::string& groupAlias);

		bool addTransaction(std::unique_ptr<model::gradido::GradidoTransaction> newTransaction, const MemoryBin* messageId, uint64_t iotaMilestoneTimestamp);

	protected:

		//! use function name from Group but also fill mRegisteredGroups
		void fillSignatureCacheOnStartup();		
		std::unordered_map<std::string, GroupEntry> mRegisteredGroups;
	};

}
#endif //__GRADIDO_NODE_CONTROLLER_GROUP_REGISTER_GROUP_H