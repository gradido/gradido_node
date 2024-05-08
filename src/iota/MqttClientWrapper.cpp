#include "MqttClientWrapper.h"
#include "MqttExceptions.h"
#include "../SingletonManager/CacheManager.h"
#include "../ServerGlobals.h"

namespace iota {
	MqttClientWrapper::MqttClientWrapper()
		: mMqttClient(nullptr), mbConnected(false), mMqttLog(Poco::Logger::get("mqttLog")), mReconnectTimeout(0)
	{
		int rc = 0;
		rc = MQTTAsync_create(&mMqttClient, ServerGlobals::g_IotaMqttBrokerUri.toString().data(), GRADIDO_NODE_MQTT_CLIENT_ID, MQTTCLIENT_PERSISTENCE_NONE, NULL);
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
		std::string causeString = "";
		if (cause) {
			causeString = std::string(cause);
		}
		mMqttLog.error("connection lost: %s", causeString);
		mReconnectTimeout = std::chrono::milliseconds(GRADIDO_NODE_MQTT_RECONNECT_START_TIMEOUT_MS);
		reconnectAttempt();
	}
	void MqttClientWrapper::connected(char* cause)
	{		
		if(cause) {
			mMqttLog.information("connected: %s", std::string(cause));
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
		mMqttLog.information("disconnected: %s", std::string(MQTTReasonCode_toString(reasonCode)));
		std::lock_guard _lock(mWorkMutex);
		mbConnected = false;
		for(auto& topicObserver: mTopicObserver) {
			topicObserver.second->setUnsubscribed();
		}
	}
	void MqttClientWrapper::onSuccess(MQTTAsync_successData* response)
	{
		mMqttLog.information("on success, token: %d", response->token);
	}
	void MqttClientWrapper::onFailure(MQTTAsync_failureData* response)
	{
		if (response->message) {
			mMqttLog.error("on failure, token: %d, code: %d, error: %s", response->token, response->code, std::string(response->message));
		}
		else {
			mMqttLog.error("on failure, token: %d, code: %d, no error message", response->token, response->code);
		}
	}
	int  MqttClientWrapper::messageArrived(char* topicName, int topicLen, MQTTAsync_message* message)
	{
		std::lock_guard _lock(mWorkMutex);
		auto it = mTopicObserver.find(topicName);
		if(it != mTopicObserver.end()) {
			it->second->messageArrived(message);
		}
		std::string payload((const char*)message->payload, message->payloadlen);
		if(0 == strcmp(topicName, "messages/indexation/484f524e4554205370616d6d6572")) {
			auto startPos = payload.find("HORNET");
			auto binDataHex = DataTypeConverter::binToHex(payload.substr(0, startPos-1));

			mMqttLog.information("message arrived for topic: %s, qos: %d, retained: %d, dup: %d, msgid: %d\n%s\n%s\n%d",
				std::string(topicName), message->qos, message->retained, message->dup, message->msgid, binDataHex, payload.substr(startPos), message->payloadlen
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
		mMqttLog.information("deliveryComplete, token: %d", token);
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
			auto &logger = mCW->getLogger();
			auto& connect = response->alt.connect;
			logger.information(
				"connected to server: %s with mqtt version: %d, session present: %d", 
				std::string(connect.serverURI), connect.MQTTVersion, connect.sessionPresent
			);
			// mCW->connected("connection call");
		};
		conn_opts.onFailure = [](void* context, MQTTAsync_failureData* response) {
			auto& logger = MqttClientWrapper::getInstance()->getLogger();
			std::string errorString("empty");
			if(response->message) {
				errorString = std::string(response->message);
			}
			logger.error("error connecting to server, error code: %d, details: %s", response->code, errorString);
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
			auto &logger = mCW->getLogger();
			logger.information("disconnect from server");
			// mCW->disconnected(nullptr, MQTTREASONCODE_ADMINISTRATIVE_ACTION);
		};
		options.onFailure = [](void* context, MQTTAsync_failureData* response) {
			auto& logger = MqttClientWrapper::getInstance()->getLogger();
			std::string errorString("empty");
			if(response->message) {
				errorString = std::string(response->message);
			}
			logger.error("couldn't disconnect from server, error code: %d, details: %s", response->code, errorString);
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