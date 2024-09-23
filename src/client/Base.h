#ifndef __GRADIDO_NODE_CLIENT_BASE_H
#define __GRADIDO_NODE_CLIENT_BASE_H

#include "gradido_blockchain/data/ConfirmedTransaction.h"

#include "Poco/URI.h"
#include "Poco/Net/NameValueCollection.h"

#include "rapidjson/document.h"

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
		Base(const Poco::URI& uri);
		virtual ~Base();

		bool notificateNewTransaction(const gradido::data::ConfirmedTransaction& confirmedTransaction);
		bool notificateFailedTransaction(
			const gradido::data::GradidoTransaction& gradidoTransaction,
			const std::string& errorMessage,
			const std::string& messageId
		);
		virtual bool postRequest(const Poco::Net::NameValueCollection& parameterValuePairs) = 0;

		inline void setGroupAlias(const std::string& groupAlias) {mGroupAlias = groupAlias;}
	protected:		
		bool notificate(Poco::Net::NameValueCollection params);
		enum class NotificationFormat : int
		{
			PROTOBUF_BASE64 = 1,
			JSON = 2
		};
		Base(const Poco::URI& uri, NotificationFormat format);

		Poco::URI mUri;
		NotificationFormat mFormat;
		std::string mGroupAlias;
	};
}

#endif //__GRADIDO_NODE_CLIENT_BASE_H