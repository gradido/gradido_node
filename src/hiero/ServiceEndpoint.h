#ifndef __GRADIDO_NODE_HIERO_SERVICE_ENDPOINT_H
#define __GRADIDO_NODE_HIERO_SERVICE_ENDPOINT_H

#include <array>
#include "gradido_blockchain/interaction/deserialize/Protopuf.h"
#include "gradido_blockchain/memory/Block.h"

namespace hiero {
    using ServiceEndpointMessage = pp::message<
        bytes_field<"ipAddressV4", 1>,
        int32_field<"port", 2>,
        string_field<"domain_name", 3>
    >;

    class ServiceEndpoint {
    public:
        ServiceEndpoint();
        ServiceEndpoint(const ServiceEndpointMessage& message);
        ServiceEndpoint(std::array<std::byte, 4> ipV4, int32_t port);
        ServiceEndpoint(int32_t port, const std::string domainName);
        ~ServiceEndpoint();

        ServiceEndpointMessage getMessage() const;

        inline int32_t getPort() const { return mPort; }
        inline std::string getDomainName() const { return mDomainName; }
        std::string getConnectionString() const;

    protected:
        std::string getIpAddressV4String() const;

        /**
         * A 32-bit IPv4 address.<br/>
         * This is the address of the endpoint, encoded in pure "big-endian"
         * (i.e. left to right) order (e.g. `127.0.0.1` has hex bytes in the
         * order `7F`, `00`, `00`, `01`).
         */
        memory::Block mIpAddressV4;
        /**
         * A TCP port to use.
         * <p>
         * This value MUST be between 0 and 65535, inclusive.
         */
        int32_t mPort;
        /**
         * A node domain name.
         * <p>
         * This MUST be the fully qualified domain name of the node.<br/>
         * This value MUST NOT exceed 253 characters.<br/>
         * When the `domain_name` field is set, the `ipAddressV4`
         * field MUST NOT be set.<br/>
         * When the `ipAddressV4` field is set, the `domain_name`
         * field MUST NOT be set.
         */
        std::string mDomainName;
    };
}
#endif // __GRADIDO_NODE_HIERO_SERVICE_ENDPOINT_H