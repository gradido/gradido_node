#include "MqttClientWrapper.h"
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
			[](void* context, char* cause) {static_cast<MqttClientWrapper*>(context)->connectionLost(cause); },
			[](void* context, char* topicName, int topicLen, MQTTAsync_message* message) -> int {return static_cast<MqttClientWrapper*>(context)->messageArrived(topicName, topicLen, message); },
			[](void* context, MQTTAsync_token token) {static_cast<MqttClientWrapper*>(context)->deliveryComplete(token); }
		);
		if (rc != MQTTASYNC_SUCCESS) {
			throw MqttSetCallbacksException("couldn't set callbacks", rc);
		}

		rc = MQTTAsync_setConnected(mMqttClient, this, [](void* context, char* cause) {static_cast<MqttClientWrapper*>(context)->connected(cause); });
		if (rc != MQTTASYNC_SUCCESS) {
			throw MqttSetCallbacksException("couldn't set connected callback", rc);
		}

		rc = MQTTAsync_setDisconnected(mMqttClient, this, [](void* context, MQTTProperties* properties, MQTTReasonCodes reasonCode) {static_cast<MqttClientWrapper*>(context)->disconnected(properties, reasonCode); });
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

	void MqttClientWrapper::subscribe(const Topic& topic, std::shared_ptr<IMessageObserver> observer)
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
	}

	void MqttClientWrapper::unsubscribe(const Topic& topic, std::shared_ptr<IMessageObserver> observer)
	{
		std::lock_guard _lock(mWorkMutex);
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
		mMqttLog.information("connected: %s", std::string(cause));
		std::lock_guard _lock(mWorkMutex);
		mbConnected = true;
		while (mWaitingTopics.size()) {
			auto topic = mWaitingTopics.front();
			mWaitingTopics.pop();
			subscribe(topic);
		}
	}
	void MqttClientWrapper::disconnected(MQTTProperties* properties, MQTTReasonCodes reasonCode)
	{
		mMqttLog.information("disconnected: %s", std::string(MQTTReasonCode_toString(reasonCode)));
		std::lock_guard _lock(mWorkMutex);
		mbConnected = false;
		for (auto topic : mTopics) {
			mWaitingTopics.push(topic);
		}
		mTopics.clear();
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
		std::string payload((const char*)message->payload, message->payloadlen);
		if(0 == strcmp(topicName, "messages/indexation/484f524e4554205370616d6d6572")) {
			auto startPos = payload.find("HORNET");
			auto binDataHex = DataTypeConverter::binToHex(payload.substr(0, startPos-1));

			mMqttLog.information("message arrived for topic: %s, qos: %d, retained: %d, dup: %d, msgid: %d\n%s\n%s\n%d",
				std::string(topicName), message->qos, message->retained, message->dup, message->msgid, binDataHex, payload.substr(startPos), message->payloadlen
			);
		} else {
			mMqttLog.information("message arrived for topic: %s, qos: %d, retained: %d, dup: %d, msgid: %d\n%s\n%d",
				std::string(topicName), message->qos, message->retained, message->dup, message->msgid, payload, message->payloadlen
			);
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

	void MqttClientWrapper::subscribe(const std::string& topic)
	{
		std::lock_guard _lock(mWorkMutex);
		if (!mbConnected) {
			mWaitingTopics.push(topic);
		}
		else {
			auto it = mTopics.insert(topic);
			if (it.second) {
				MQTTAsync_responseOptions options = MQTTAsync_responseOptions_initializer;
				options.context = this;
				options.onSuccess = [](void* context, MQTTAsync_successData* response) {static_cast<MqttClientWrapper*>(context)->onSuccess(response); };
				options.onFailure = [](void* context, MQTTAsync_failureData* response) {static_cast<MqttClientWrapper*>(context)->onFailure(response); };
				auto rc = MQTTAsync_subscribe(mMqttClient, topic.data(), 1, &options);

				if (rc != MQTTASYNC_SUCCESS) {
					throw MqttSubscribeException("error subscribing", rc, topic);
				}
				mMqttLog.information("subscribe for topic: %s", topic);
			}
		}
	}

	void MqttClientWrapper::unsubscribe(const std::string& topic)
	{
		std::lock_guard _lock(mWorkMutex);
		auto it = mTopics.find(topic);
		if (it == mTopics.end()) {
			throw MqttSubscribeException("topic not found for unsubscribe", 0, topic);
		}
		mTopics.erase(it);
		if (mbConnected) {
			MQTTAsync_responseOptions options = MQTTAsync_responseOptions_initializer;
			options.context = this;
			options.onSuccess = [](void* context, MQTTAsync_successData* response) {static_cast<MqttClientWrapper*>(context)->onSuccess(response); };
			options.onFailure = [](void* context, MQTTAsync_failureData* response) {static_cast<MqttClientWrapper*>(context)->onFailure(response); };
			auto rc = MQTTAsync_unsubscribe(mMqttClient, topic.data(), &options);

			if (rc != MQTTASYNC_SUCCESS) {
				throw MqttSubscribeException("error unsubscribing", rc, topic);
			}
			mMqttLog.information("subscribe for topic: %s", topic);
		}
	}

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
		conn_opts.onSuccess = [](void* context, MQTTAsync_successData* response) {static_cast<MqttClientWrapper*>(context)->onSuccess(response); };
		conn_opts.onFailure = [](void* context, MQTTAsync_failureData* response) {static_cast<MqttClientWrapper*>(context)->onFailure(response); };
		auto rc = MQTTAsync_connect(mMqttClient, &conn_opts);

		if (rc != MQTTASYNC_SUCCESS) {
			throw MqttConnectionException("failed to connect", rc, ServerGlobals::g_IotaMqttBrokerUri);
		}
	}

	void MqttClientWrapper::disconnect()
	{
		MQTTAsync_disconnectOptions options = MQTTAsync_disconnectOptions_initializer;
		options.context = this;
		options.onSuccess = [](void* context, MQTTAsync_successData* response) {static_cast<MqttClientWrapper*>(context)->onSuccess(response); };
		options.onFailure = [](void* context, MQTTAsync_failureData* response) {static_cast<MqttClientWrapper*>(context)->onFailure(response); };
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

	// +++++++++++++++ Exceptions ++++++++++++++++++++++
	std::string MqttException::getFullString() const
	{
		std::string result = what();
		result += ", error code: " + std::to_string(mErrorCode);
		return std::move(result);
	}

	std::string MqttConnectionException::getFullString() const
	{
		std::string result = MqttException::getFullString();
		result += ", uri: " + mUri.toString();
		return std::move(result);
	}

	std::string MqttSubscribeException::getFullString() const
	{
		std::string result = MqttException::getFullString();
		result += ", topic: " + mTopic;
		return std::move(result);
	}
}