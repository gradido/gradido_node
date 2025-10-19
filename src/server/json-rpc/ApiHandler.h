#ifndef __GRADIDO_NODE_SERVER_JSON_RPC_API_HANDLER_H
#define __GRADIDO_NODE_SERVER_JSON_RPC_API_HANDLER_H

#include "RequestHandler.h"
#include "gradido_blockchain/types.h"

namespace gradido {
	namespace blockchain {
		class Abstract;
		class Filter;
	}
}

namespace server {
	namespace json_rpc {

		// TODO: write api doc and help on command
		class ApiHandler : public RequestHandler
		{
		public:
			void handle(rapidjson::Value& responseJson, std::string method, const rapidjson::Value& params);

		protected:
		  void listCommunities(rapidjson::Value& resultJson);
			/*
			*
			curl -X POST -H "Content-Type: application/json" -d '{"jsonrpc": "2.0", "method": "getTransactions", "params": {"groupAlias": "e78c6a06f4efdde1a93d061fe2d89179e093d901a02379a8a2319822f4f3ce71","fromTransactionId": "1","format": "json"},"id":1 }' http://localhost:8340

			*/
			void findAllTransactions(
				rapidjson::Value& resultJson,
				const gradido::blockchain::Filter& filter,
				std::shared_ptr<gradido::blockchain::Abstract> blockchain,
				const std::string& format
			);
			/*!
			* TODO: implement index for iota message id if it is used much
			* @param resultJson: for success result
			* @param responseJson: for the overall response, used for example for errors
			* @param transactionId: this parameter or
			* @param iotaMessageId: this parameter for finding transaction
			*/
			void getTransaction(
				rapidjson::Value& resultJson,
				rapidjson::Value& responseJson,
				std::shared_ptr<gradido::blockchain::Abstract> blockchain,
				const std::string& format,
				uint64_t transactionId = 0,
				std::shared_ptr<memory::Block> iotaMessageId = nullptr
			);
			//! \param searchStartDate start date for reverse search for creation transactions range -2 month from there
			void getCreationSumForMonth(
				rapidjson::Value& resultJson,
				memory::ConstBlockPtr pubkey,
				Timepoint targetDate,
				Timepoint transactionCreationDate,
				std::shared_ptr<gradido::blockchain::Abstract> blockchain
			);
			void getAddressBalance(
				rapidjson::Value& resultJson,
				memory::ConstBlockPtr pubkey,
				Timepoint date,
				std::shared_ptr<gradido::blockchain::Abstract> blockchain,
				const std::string& coinCommunityId = ""
			);
			void getAddressType(
				rapidjson::Value& resultJson,
				memory::ConstBlockPtr pubkey,
				std::shared_ptr<gradido::blockchain::Abstract> blockchain
			);
			void getAddressTxids(
				rapidjson::Value& resultJson,
				memory::ConstBlockPtr pubkey,
				std::shared_ptr<gradido::blockchain::Abstract> blockchain
			);
			void listTransactions(
				rapidjson::Value& resultJson,
				std::shared_ptr<gradido::blockchain::Abstract> blockchain,
				const gradido::blockchain::Filter& filter
			);
			void getTransactionsForAddress(
				rapidjson::Value& resultJson,
				memory::ConstBlockPtr pubkey,
				uint64_t firstTransactionNr,
				uint32_t maxResultCount,
				std::shared_ptr<gradido::blockchain::Abstract> blockchain
			);
			void findUserByNameHash(
				rapidjson::Value& resultJson,
				rapidjson::Value& responseJson,
				memory::ConstBlockPtr nameHash,
				std::shared_ptr<gradido::blockchain::Abstract> blockchain
			);
			// helper	

		};
	}
}

#endif // __GRADIDO_NODE_SERVER_JSON_RPC_API_HANDLER_H
