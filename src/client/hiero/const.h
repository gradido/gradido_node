#ifndef __GRADIDO_NODE_CLIENT_HIERO_CONST_H_
#define __GRADIDO_NODE_CLIENT_HIERO_CONST_H_

namespace hiero {
    static constexpr unsigned int PORT_MIRROR_PLAIN = 5600U;
    static constexpr unsigned int PORT_MIRROR_TLS = 443U;
    static constexpr unsigned int PORT_NODE_PLAIN = 50211U;
    static constexpr unsigned int PORT_NODE_TLS = 50212U;
    /**
     * The default minimum duration of time to wait before retrying to submit a previously-failed request.
     */
    constexpr auto DEFAULT_MIN_BACKOFF = std::chrono::milliseconds(250);
    /**
     * The default maximum duration of time to wait before retrying to submit a previously-failed request.
     */
    constexpr auto DEFAULT_MAX_BACKOFF = std::chrono::seconds(8);
}

#endif // __GRADIDO_NODE_CLIENT_HIERO_CONST_H_