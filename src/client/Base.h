#ifndef __GRADIDO_NODE_CLIENT_BASE_H
#define __GRADIDO_NODE_CLIENT_BASE_H

#include "Poco/URI.h"
#include "Poco/Net/NameValueCollection.h"

#include "rapidjson/document.h"
#include "gradido_blockchain/model/protobufWrapper/GradidoBlock.h"

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

		bool notificateNewTransaction(Poco::SharedPtr<model::gradido::GradidoBlock> gradidoBlock);
		virtual bool postRequest(const Poco::Net::NameValueCollection& parameterValuePairs) = 0;
	protected:		
		enum NotificationFormat : int
		{
			NOTIFICATION_FORMAT_PROTOBUF_BASE64,
			NOTIFICATION_FORMAT_JSON
		};
		Base(const Poco::URI& uri, NotificationFormat format);

		Poco::URI mUri;
		NotificationFormat mFormat;

	};
}

#endif //__GRADIDO_NODE_CLIENT_BASE_H