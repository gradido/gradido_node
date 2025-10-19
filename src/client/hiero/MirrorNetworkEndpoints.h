#ifndef __GRADIDO_NODE_CLIENT_HIERO_MIRROR_NETWORK_ENDPOINTS_H
#define __GRADIDO_NODE_CLIENT_HIERO_MIRROR_NETWORK_ENDPOINTS_H

#include <cstring>

namespace client {
    namespace hiero {
        namespace MirrorNetworkEndpoints {
            static constexpr const char* TESTNET = "testnet.mirrornode.hedera.com";
            static constexpr const char* PREVIEWNET = "previewnet.mirrornode.hedera.com";
            static constexpr const char* MAINNET = "mainnet-public.mirrornode.hedera.com";

            const char* getByEndpointName(const char* endpointName) noexcept;
        }
    }
}
#endif // __GRADIDO_NODE_CLIENT_HIERO_MIRROR_NETWORK_ENDPOINTS_H