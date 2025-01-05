#ifndef __GRADIDO_NODE_IOTA_MQTT_RESPONSE_H
#define __GRADIDO_NODE_IOTA_MQTT_RESPONSE_H

#include "MQTTAsync.h"

namespace iota {

    class MqttResponse
    {
    public:
        virtual void onSuccess(MQTTAsync_successData* response) = 0;
		virtual void onFailure(MQTTAsync_failureData* response) = 0;

    protected:

    };
}

#endif //__GRADIDO_NODE_IOTA_MQTT_RESPONSE_H