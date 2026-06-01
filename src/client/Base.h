#ifndef __GRADIDO_NODE_CLIENT_BASE_H
#define __GRADIDO_NODE_CLIENT_BASE_H

#include "gradido_blockchain/data/ConfirmedTransaction.h"
#include "gradido_blockchain/data/ByteArray.h"

#include "rapidjson/document.h"

namespace hiero {
	class TransactionId;
}

namespace client
{
	/*
		@author: einhornimmond

		@date: 10.03.2022

		@brief: Base Class for client used from Gradido Node if it sends proactive messages (requests) to community server

		Used for different receiver formats
		like graphQL or Json
	*/

	class Base
	{
	public:
		Base(const std::string& successUrl, const std::string& failedUrl);
		virtual ~Base();

		bool notificateNewTransaction(const gradido::data::ConfirmedTransaction& confirmedTransaction);
		bool notificateFailedTransaction(
			const gradido::data::GradidoTransaction& gradidoTransaction,
			const std::string& errorMessage,
			const ::hiero::TransactionId& hieroTransactionId
		);
		virtual bool postRequest(const std::map<std::string, std::string>& parameterValuePairs, const std::string& url) = 0;
		inline void setCommunityUuid(const std::string& communityUuid) { mCommunityUuid = communityUuid; }

	protected:
		bool notificate(const std::map<std::string, std::string>& params, const std::string& url);
		enum class NotificationFormat : int
		{
			PROTOBUF_BASE64 = 1,
			JSON = 2
		};
		Base(const std::string& successUrl, const std::string& failedUrl, NotificationFormat format);

		const std::string mSuccessUrl;
		const std::string mFailedUrl;
		NotificationFormat mFormat;
		std::string mCommunityUuid;
	};
}

#endif //__GRADIDO_NODE_CLIENT_BASE_H
