#ifndef __GRADIDO_NODE_IOTA_MQTT_EXCEPTIONS 
#define __GRADIDO_NODE_IOTA_MQTT_EXCEPTIONS 

#include "gradido_blockchain/GradidoBlockchainException.h"

namespace iota {

	class MqttException : public GradidoBlockchainException
	{
	public: 
		explicit MqttException(const char* what, int errorCode) noexcept : GradidoBlockchainException(what), mErrorCode(errorCode) {}
		std::string getFullString() const;
	protected:
		int mErrorCode;
	};

	class MqttCreationException : public MqttException
	{
	public:
		explicit MqttCreationException(const char* what, int errorCode) noexcept : MqttException(what, errorCode) {}
	};

	class MqttSetCallbacksException : public MqttException
	{
	public: 
		explicit MqttSetCallbacksException(const char* what, int errorCode) noexcept : MqttException(what, errorCode) {}
	};

	class MqttConnectionException : public MqttException
	{
	public:
		explicit MqttConnectionException(const char* what, int errorCode, std::string uri) noexcept : MqttException(what, errorCode), mUri(uri) {}
		std::string getFullString() const;

	protected:
		std::string mUri;
	};

	class MqttSubscribeException : public MqttException
	{
	public:
		explicit MqttSubscribeException(const char* what, int errorCode, const std::string& topic) noexcept : MqttException(what, errorCode), mTopic(topic) {}
		std::string getFullString() const;

	protected:
		std::string mTopic;
	};
}

#endif //__GRADIDO_NODE_IOTA_MQTT_EXCEPTIONS 