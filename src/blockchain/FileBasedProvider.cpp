#include "FileBasedProvider.h"
#include "../SystemExceptions.h"
#include "../client/JsonRPC.h"
#include "../client/GraphQL.h"
#include "../ServerGlobals.h"

#include "loguru/loguru.hpp"

namespace gradido {
	namespace blockchain {
		FileBasedProvider::FileBasedProvider()
			:mGroupIndex(nullptr), mCommunityIdIndex(ServerGlobals::g_FilesPath + "/communityId.index"), mInitalized(false)
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
			mCommunityIdIndex.exit();
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
			return nullptr;
		}
		bool FileBasedProvider::init(const std::string& communityConfigFile)
		{
			std::lock_guard _lock(mWorkMutex);
			mInitalized = true;
			bool resetAllCommunityIndices = false;
			if (!mCommunityIdIndex.init()) {
				mCommunityIdIndex.reset();
				resetAllCommunityIndices = true;
			}
			mGroupIndex = new cache::GroupIndex(communityConfigFile);
			mGroupIndex->update();
			auto communitiesAlias = mGroupIndex->listGroupAliases();
			for (auto& communityAlias : communitiesAlias) {
				// exit if at least one blockchain from config couldn't be loaded
				// should only occure with invalid config
				if (!addCommunity(communityAlias, resetAllCommunityIndices)) {
					return false;
				}
			}
			return true;
		}
		void FileBasedProvider::exit()
		{
			std::lock_guard _lock(mWorkMutex);
			mInitalized = false;
			clear();
		}

		int FileBasedProvider::reloadConfig()
		{
			std::lock_guard _lock(mWorkMutex);
			if (!mInitalized) {
				throw ClassNotInitalizedException("please call init before", "blockchain::FileBasedProvider");
			}
			mGroupIndex->update();
			auto communitiesAlias = mGroupIndex->listGroupAliases();
			int addedBlockchainsCount = 0;
			for (auto& communityAlias : communitiesAlias) {
				auto it = mBlockchainsPerGroup.find(communityAlias);
				if (it == mBlockchainsPerGroup.end()) {
					if(addCommunity(communityAlias, false)) {
						addedBlockchainsCount++;
					}
				}
				else {
					updateListenerCommunity(communityAlias, it->second);
				}
			}
			return addedBlockchainsCount;
		}

		

		std::shared_ptr<FileBased> FileBasedProvider::addCommunity(const std::string& communityAlias, bool resetIndices)
		{
			try {
				auto folder = mGroupIndex->getFolder(communityAlias);

				// with that call community will be initialized and start listening
				auto blockchain = std::make_shared<FileBased>(communityAlias, folder);
				updateListenerCommunity(communityAlias, blockchain);
				if (!blockchain->init()) {
					LOG_F(ERROR, "error initalizing blockchain: %s", communityAlias.data());
					return nullptr;
				}
				mBlockchainsPerGroup.insert({ communityAlias, blockchain });
				return blockchain;
			}
			catch (GradidoBlockchainException& ex) {
				LOG_F(ERROR, "gradido blockchain exception: %s in community : %s",
					ex.getFullString().data(),
					communityAlias.data()
				);
				return nullptr;
			}
		}
		void FileBasedProvider::updateListenerCommunity(const std::string& alias, std::shared_ptr<FileBased> blockchain)
		{
			const auto& communityConfig = mGroupIndex->getCommunityDetails(alias);
			// for notification of community server by new transaction
			// deprecated, will be replaced with mqtt in future
			if (!communityConfig.newBlockUri.empty()) {
				std::shared_ptr<client::Base> clientBase;
				auto uri = Poco::URI(communityConfig.newBlockUri);
				if (communityConfig.blockUriType == "json") {
					clientBase = std::make_shared<client::JsonRPC>(uri);
				}
				else if (communityConfig.blockUriType == "graphql") {
					clientBase = std::make_shared<client::GraphQL>(uri);
				}
				else {
					LOG_F(ERROR, "unknown new block uri type: %s", communityConfig.blockUriType.data());
					return;
				}
				if (clientBase) {
					clientBase->setGroupAlias(alias);
					blockchain->setListeningCommunityServer(clientBase);
					LOG_F(INFO, "notification of community: %s", alias.data());
				}
			}
		}
	}
}