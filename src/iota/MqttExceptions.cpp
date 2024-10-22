#include "MqttExceptions.h"

namespace iota {

	std::string MqttException::getFullString() const
	{
		std::string result = what();
		result += ", error code: " + std::to_string(mErrorCode);
		return std::move(result);
	}

	std::string MqttConnectionException::getFullString() const
	{
		std::string result = MqttException::getFullString();
		result += ", uri: " + mUri;
		return std::move(result);
	}

	std::string MqttSubscribeException::getFullString() const
	{
		std::string result = MqttException::getFullString();
		result += ", topic: " + mTopic;
		return std::move(result);
	}
}