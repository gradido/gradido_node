#include "FileBasedProvider.h"
#include "../SystemExceptions.h"
#include "../client/JsonRPC.h"
#include "../client/GraphQL.h"

namespace gradido {
	namespace blockchain {
		FileBasedProvider::FileBasedProvider()
			:mGroupIndex(nullptr), mInitalized(false)
		{

		}

		FileBasedProvider::~FileBasedProvider()
		{
			std::lock_guard _lock(mWorkMutex);
			clear();
			if (mGroupIndex) {
				delete mGroupIndex;
				mGroupIndex = nullptr;
			}
		}

		void FileBasedProvider::clear()
		{
			mBlockchainsPerGroup.clear();
		}

		FileBasedProvider* FileBasedProvider::getInstance()
		{
			FileBasedProvider one;
			return &one;
		}

		std::shared_ptr<Abstract> FileBasedProvider::findBlockchain(std::string_view communityId)
		{
			std::lock_guard _lock(mWorkMutex);
			if (!mInitalized) {
				throw ClassNotInitalizedException("please call init before", "blockchain::FileBasedProvider");
			}
			auto it = mBlockchainsPerGroup.find(communityId);
			if (it != mBlockchainsPerGroup.end()) {
				return it->second;
			}
			// load new blockchain if it exist in config
		}
		void FileBasedProvider::init(const std::string& communityConfigFile)
		{
			std::lock_guard _lock(mWorkMutex);
			mInitalized = true;
			mGroupIndex = new controller::GroupIndex(communityConfigFile);
			mGroupIndex->update();
			auto communitiesAlias = mGroupIndex->listGroupAliases();
			for (auto& communityAlias : communitiesAlias) {
				// load all groups to start iota message listener from all groups
				try {
					
					auto folder = mGroupIndex->getFolder(communityAlias);
					const auto& communityConfig = mGroupIndex->getCommunityDetails(communityAlias);
					// with that call community will be initialized and start listening
					addCommunity(communityAlias, folder);

					// for notification of community server by new transaction
					// deprecated, will be replaced with mqtt in future
					if (!communityConfig.newBlockUri.empty()) {
						client::Base* clientBase = nullptr;
						auto uri = Poco::URI(communityConfig.newBlockUri);
						if (communityConfig.blockUriType == "json") {
							clientBase = new client::JsonRPC(uri);
						} else if (communityConfig.blockUriType == "graphql") {
							clientBase = new client::GraphQL(uri);
						}
						else {
							errorLog.error("unknown new block uri type: %s", communityNewBlockUriType);
						}
						if (clientBase) {
							clientBase->setGroupAlias(*it);
							group->setListeningCommunityServer(clientBase);
							errorLog.information("[GroupManager::init] notification of community: %s", *it);
						}
					}
				}
				catch (GradidoBlockchainTransactionNotFoundException& ex) {
					errorLog.error("[GroupManager::init] transaction not found exception: %s in group: %s", ex.getFullString(), *it);
					return -1;
				}
				catch (GradidoBlockchainException& ex) {
					errorLog.error("[GroupManager::init] gradido blockchain exception: %s in group: %s", ex.getFullString(), *it);
					return -2;
				}
			}
		}
		void FileBasedProvider::exit()
		{
			std::lock_guard _lock(mWorkMutex);
			mInitalized = false;
			clear();
		}

		void FileBasedProvider::addCommunity(const std::string& alias, const std::string& folder)
		{

		}
	}
}