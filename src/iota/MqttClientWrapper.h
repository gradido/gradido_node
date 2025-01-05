#ifndef __GRADIDO_NODE_IOTA_MQTT_CLIENT_WRAPPER_H
#define __GRADIDO_NODE_IOTA_MQTT_CLIENT_WRAPPER_H

#include "MQTTAsync.h"

#include "gradido_blockchain/GradidoBlockchainException.h"

#include "../lib/FuzzyTimer.h"
#include "TopicObserver.h"

#include <mutex>
#include <set>
#include <queue>
#include <unordered_map>

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
		void exit();

		void subscribe(const Topic& topic, IMessageObserver* observer);
		void unsubscribe(const Topic& topic, IMessageObserver* observer);
		void removeTopicObserver(const std::string& topicString);


		// callbacks called from MQTT
		void connectionLost(char* cause);
		void connected(char* cause);
		void disconnected(MQTTProperties* properties, MQTTReasonCodes reasonCode);
		void onSuccess(MQTTAsync_successData* response);
		void onFailure(MQTTAsync_failureData* response);
		int  messageArrived(char* topicName, int topicLen, MQTTAsync_message* message);
		void deliveryComplete(MQTTAsync_token token);

		inline bool isConnected() const { return mbConnected; }

		const char* getResourceType() const { return "MqttClientWrapper"; }
	
		TimerReturn callFromTimer();

		TopicObserver* findTopicObserver(const char* topicString);

	protected:
		void connect();
		void disconnect();
		void reconnectAttempt();

		MqttClientWrapper();
		MQTTAsync mMqttClient;
		std::recursive_mutex mWorkMutex;
		// member bool connected
		bool mbConnected;
		//! topic string as key
		std::unordered_map<std::string, std::unique_ptr<TopicObserver>> mTopicObserver;

		// for automatic reconnect attempts
		std::chrono::milliseconds mReconnectTimeout;

	};


}

#endif //__GRADIDO_NODE_IOTA_MQTT_CLIENT_WRAPPER_H