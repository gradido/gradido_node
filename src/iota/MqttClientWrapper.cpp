#include "MqttClientWrapper.h"
#include "MqttExceptions.h"
#include "../SingletonManager/CacheManager.h"
#include "../ServerGlobals.h"

#include "loguru/loguru.hpp"

namespace iota {
	MqttClientWrapper::MqttClientWrapper()
		: mMqttClient(nullptr), mbConnected(false), mReconnectTimeout(GRADIDO_NODE_MQTT_RECONNECT_START_TIMEOUT_MS)
	{
		int rc = 0;
		rc = MQTTAsync_create(&mMqttClient, ServerGlobals::g_IotaMqttBrokerUri.data(), GRADIDO_NODE_MQTT_CLIENT_ID, MQTTCLIENT_PERSISTENCE_NONE, NULL);
		if (rc != MQTTASYNC_SUCCESS) {
			throw MqttCreationException("couldn't create mqtt client", rc);
		}

		rc = MQTTAsync_setCallbacks(
				mMqttClient,
				this,
				[](void *context, char *cause)
				{ MqttClientWrapper::getInstance()->connectionLost(cause); },
				[](void *context, char *topicName, int topicLen, MQTTAsync_message *message) -> int
				{ return MqttClientWrapper::getInstance()->messageArrived(topicName, topicLen, message); },
				[](void *context, MQTTAsync_token token)
				{ MqttClientWrapper::getInstance()->deliveryComplete(token); });
		if (rc != MQTTASYNC_SUCCESS) {
			throw MqttSetCallbacksException("couldn't set callbacks", rc);
		}

		rc = MQTTAsync_setConnected(mMqttClient, this, [](void *context, char *cause)
																{ MqttClientWrapper::getInstance()->connected(cause); });
		if (rc != MQTTASYNC_SUCCESS) {
			throw MqttSetCallbacksException("couldn't set connected callback", rc);
		}

		rc = MQTTAsync_setDisconnected(mMqttClient, this, [](void *context, MQTTProperties *properties, MQTTReasonCodes reasonCode)
																	 { MqttClientWrapper::getInstance()->disconnected(properties, reasonCode); });
		if (rc != MQTTASYNC_SUCCESS) {
			throw MqttSetCallbacksException("couldn't set disconnected callback", rc);
		}

	}

	MqttClientWrapper::~MqttClientWrapper()
	{
		if (mMqttClient) {
			disconnect();
			MQTTAsync_destroy(&mMqttClient);
			mMqttClient = nullptr;
		}
	}

	MqttClientWrapper* MqttClientWrapper::getInstance()
	{
		static MqttClientWrapper one;
		return &one;
	}

	bool MqttClientWrapper::init()
	{
		try {
			connect();
		}
		catch (MqttConnectionException& ex) {
			reconnectAttempt();
		}
		return true;
	}

	void MqttClientWrapper::subscribe(const Topic& topic, IMessageObserver* observer)
	{
		std::lock_guard _lock(mWorkMutex);
		auto it = mTopicObserver.find(topic.getTopicString());
		if(it == mTopicObserver.end()) {
			auto result = mTopicObserver.insert({topic.getTopicString(), std::make_unique<TopicObserver>(topic)});
			if(result.second) {
				it = result.first;
			} else {
				throw std::runtime_error("error inserting missing topic observer");
			}
		} 
		if(mbConnected && it->second->isUnsubscribed()) {
			it->second->subscribe(mMqttClient);
		}	
		it->second->addObserver(observer);
	}

	void MqttClientWrapper::unsubscribe(const Topic& topic, IMessageObserver* observer)
	{
		std::lock_guard _lock(mWorkMutex);
		auto it = mTopicObserver.find(topic.getTopicString());
		if(it != mTopicObserver.end()) {
			auto observerCount = it->second->removeObserver(observer);
			if(!observerCount && mbConnected) {
				it->second->unsubscribe(mMqttClient);
			}
		}
	}
	void MqttClientWrapper::removeTopicObserver(const std::string& topicString)
	{
		std::lock_guard _lock(mWorkMutex);
		auto it = mTopicObserver.find(topicString);
		if(it != mTopicObserver.end()) {
			if(it->second->isUnsubscribed() && !it->second->getObserverCount()) {
				mTopicObserver.erase(it);
			}
		}
	}

	// ------- Callback functions -------
	void MqttClientWrapper::connectionLost(char* cause)
	{
		loguru::set_thread_name("MQTTClient");
		std::string causeString = "";
		if (cause) {
			causeString = std::string(cause);
		}
		LOG_F(ERROR, "connection lost: %s", causeString.data());
		std::lock_guard _lock(mWorkMutex);
		mbConnected = false;
		for (auto &topicObserver : mTopicObserver) {
			topicObserver.second->setUnsubscribed();
		}
		mReconnectTimeout = std::chrono::milliseconds(GRADIDO_NODE_MQTT_RECONNECT_START_TIMEOUT_MS);
		reconnectAttempt();
	}
	void MqttClientWrapper::connected(char* cause)
	{		
		loguru::set_thread_name("MQTTClient");
		if(cause) {
			LOG_F(INFO, "connected: %s", cause);
		}
		std::lock_guard _lock(mWorkMutex);
		mbConnected = true;
		for(auto& topicObserver: mTopicObserver) {
			if(topicObserver.second->getObserverCount()) {
				topicObserver.second->subscribe(mMqttClient);
			}
		}
	}
	void MqttClientWrapper::disconnected(MQTTProperties* properties, MQTTReasonCodes reasonCode)
	{
		loguru::set_thread_name("MQTTClient");
		LOG_F(INFO, "disconnected: %s", MQTTReasonCode_toString(reasonCode));
		std::lock_guard _lock(mWorkMutex);
		mbConnected = false;
		for(auto& topicObserver: mTopicObserver) {
			topicObserver.second->setUnsubscribed();
		}
	}
	void MqttClientWrapper::onSuccess(MQTTAsync_successData* response)
	{
		loguru::set_thread_name("MQTTClient");
		LOG_F(INFO, "on success, token: %d", response->token);
	}
	void MqttClientWrapper::onFailure(MQTTAsync_failureData* response)
	{
		loguru::set_thread_name("MQTTClient");
		if (response->message) {
			LOG_F(ERROR, "on failure, token: %d, code: %d, error: %s", response->token, response->code, response->message);
		}
		else {
			LOG_F(ERROR, "on failure, token: %d, code: %d, no error message", response->token, response->code);
		}
	}
	int  MqttClientWrapper::messageArrived(char* topicName, int topicLen, MQTTAsync_message* message)
	{
		loguru::set_thread_name("MQTTClient");
		std::lock_guard _lock(mWorkMutex);
		auto it = mTopicObserver.find(topicName);
		if(it != mTopicObserver.end()) {
			it->second->messageArrived(message);
		}
		std::string payload((const char*)message->payload, message->payloadlen);
		if(0 == strcmp(topicName, "messages/indexation/484f524e4554205370616d6d6572")) {
			auto startPos = payload.find("HORNET");
			auto binDataHex = DataTypeConverter::binToHex(payload.substr(0, startPos-1));

			LOG_F(INFO, "message arrived for topic: %s, qos: %d, retained: %d, dup: %d, msgid: %d\n%s\n%s\n%d",
				topicName, message->qos, message->retained, message->dup, message->msgid, binDataHex.data(), payload.substr(startPos).data(), message->payloadlen
			);
		} else {
			/*mMqttLog.information("message arrived for topic: %s, qos: %d, retained: %d, dup: %d, msgid: %d\n%s\n%d",
				std::string(topicName), message->qos, message->retained, message->dup, message->msgid, payload, message->payloadlen
			);*/
		}
		MQTTAsync_freeMessage(&message);
		MQTTAsync_free(topicName);
		return true;
	}
	void MqttClientWrapper::deliveryComplete(MQTTAsync_token token)
	{
		loguru::set_thread_name("MQTTClient");
		LOG_F(INFO, "deliveryComplete, token: %d", token);
	}

	// ------- Callback functions end -------

	TimerReturn MqttClientWrapper::callFromTimer()
	{
		if (!mbConnected) {
			try {
				connect();
			}
			catch (MqttConnectionException& ex) {
				reconnectAttempt();
			}
		}
		return TimerReturn::REMOVE_ME;
	}

	void MqttClientWrapper::connect()
	{
		MQTTAsync_connectOptions conn_opts = MQTTAsync_connectOptions_initializer;
		conn_opts.keepAliveInterval = 20;
		conn_opts.cleansession = 1;
		conn_opts.context = this;
		conn_opts.onSuccess = [](void *context, MQTTAsync_successData *response)
		{
			auto mCW = MqttClientWrapper::getInstance();
			auto& connect = response->alt.connect;
			LOG_F(INFO, "connected to server: %s with mqtt version: %d, session present: %d",
				connect.serverURI, connect.MQTTVersion, connect.sessionPresent
			);
			// mCW->connected("connection call");
		};
		conn_opts.onFailure = [](void* context, MQTTAsync_failureData* response) {
			std::string errorString("empty");
			if(response->message) {
				errorString = std::string(response->message);
			}
			LOG_F(ERROR, "error connecting to server, error code: %d, details: %s", response->code, errorString.data());
		};
		auto rc = MQTTAsync_connect(mMqttClient, &conn_opts);

		if (rc != MQTTASYNC_SUCCESS) {
			throw MqttConnectionException("failed to connect", rc, ServerGlobals::g_IotaMqttBrokerUri);
		}
	}

	void MqttClientWrapper::disconnect()
	{
		MQTTAsync_disconnectOptions options = MQTTAsync_disconnectOptions_initializer;
		options.context = this;
		options.onSuccess = [](void *context, MQTTAsync_successData *response)
		{
			auto mCW = MqttClientWrapper::getInstance();
			LOG_F(INFO, "disconnect from iota server");
			// mCW->disconnected(nullptr, MQTTREASONCODE_ADMINISTRATIVE_ACTION);
		};
		options.onFailure = [](void* context, MQTTAsync_failureData* response) {
			std::string errorString("empty");
			if(response->message) {
				errorString = std::string(response->message);
			}
			LOG_F(ERROR, "couldn't disconnect from iota server, error code: %d, details: %s", response->code, errorString.data());
		};
		auto rc = MQTTAsync_disconnect(mMqttClient, &options);

		if (rc != MQTTASYNC_SUCCESS) {
			throw MqttConnectionException("failed to disconnect", rc, ServerGlobals::g_IotaMqttBrokerUri);
		}
	}

	void MqttClientWrapper::reconnectAttempt()
	{
		std::lock_guard _lock(mWorkMutex);
		if (mbConnected) return;
		CacheManager::getInstance()->getFuzzyTimer()->addTimer(
			GRADIDO_NODE_MQTT_TIMER_NAME, this, mReconnectTimeout, 0
		);
	}

	TopicObserver *MqttClientWrapper::findTopicObserver(const char* topicString)
	{
		auto it = mTopicObserver.find(topicString);
		if(it != mTopicObserver.end()) {
			return it->second.get();
		}
		return nullptr;
	}
}