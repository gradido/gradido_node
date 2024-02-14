#ifndef __GRADIDO_NODE_IOTA_MQTT_CLIENT_WRAPPER_H
#define __GRADIDO_NODE_IOTA_MQTT_CLIENT_WRAPPER_H

#include "MQTTAsync.h"
#include "gradido_blockchain/GradidoBlockchainException.h"
#include "../lib/FuzzyTimer.h"
#include "Poco/URI.h"
#include "Poco/Logger.h"
#include <mutex>
#include <set>
#include <queue>

#define GRADIDO_NODE_MQTT_CLIENT_ID "Gradido Node Server C++"
#define GRADIDO_NODE_MQTT_RECONNECT_START_TIMEOUT_MS 200 
#define GRADIDO_NODE_MQTT_TIMER_NAME "MqttReconnectTimer"

namespace iota
{
	/*!
	*  @author einhornimmond
	*  @date 22.02.2023
	*  @brief manage mqtt client 
	*/
	class MqttClientWrapper: public TimerCallback
	{
	public:
		~MqttClientWrapper();
		static MqttClientWrapper* getInstance();

		bool init();

		// callbacks called from MQTT
		void connectionLost(char* cause);
		void connected(char* cause);
		void disconnected(MQTTProperties* properties, MQTTReasonCodes reasonCode);
		void onSuccess(MQTTAsync_successData* response);
		void onFailure(MQTTAsync_failureData* response);
		int  messageArrived(char* topicName, int topicLen, MQTTAsync_message* message);
		void deliveryComplete(MQTTAsync_token token);

		void subscribe(const std::string& topic);
		void unsubscribe(const std::string& topic);

		inline bool isConnected() const { return mbConnected; }

		const char* getResourceType() const { return "MqttClientWrapper"; }

		
		TimerReturn callFromTimer();

	protected:
		void connect();
		void disconnect();
		void reconnectAttempt();

		MqttClientWrapper();
		MQTTAsync mMqttClient;
		std::recursive_mutex mWorkMutex;
		// member bool connected
		bool mbConnected;
		std::set<std::string> mTopics;
		//! waiting for subscribing
		std::queue<std::string> mWaitingTopics;
		Poco::Logger& mMqttLog;

		// for automatic reconnect attempts
		std::chrono::milliseconds mReconnectTimeout;

	};

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
		explicit MqttConnectionException(const char* what, int errorCode, Poco::URI uri) noexcept : MqttException(what, errorCode), mUri(uri) {}
		std::string getFullString() const;

	protected:
		Poco::URI mUri;
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

#endif //__GRADIDO_NODE_IOTA_MQTT_CLIENT_WRAPPER_H