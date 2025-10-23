#include "ServiceEndpoint.h"
#include "gradido_blockchain/GradidoBlockchainException.h"

using namespace pp;

namespace hiero {
	ServiceEndpoint::ServiceEndpoint()
		: mIpAddressV4(0), mPort(0)
	{

	}

	ServiceEndpoint::ServiceEndpoint(const ServiceEndpointMessage& message)
		: mIpAddressV4(0)
	{
		if (message["ipAddressV4"_f].has_value()) {
			mIpAddressV4 = memory::Block(message["ipAddressV4"_f].value());
		}
		else {
			mDomainName = message["domain_name"_f].value();
		}
		mPort = message["port"_f].value();
	}

	
	ServiceEndpoint::ServiceEndpoint(std::array<std::byte, 4> ipV4, int32_t port)
		: mIpAddressV4(std::span<std::byte, 4>(ipV4)), mPort(port)
	{

	}
	
	ServiceEndpoint::ServiceEndpoint(int32_t port, const std::string domainName)
		: mIpAddressV4(0), mPort(port), mDomainName(domainName)
	{

	}

	ServiceEndpoint::~ServiceEndpoint()
	{

	}

	ServiceEndpointMessage ServiceEndpoint::getMessage() const
	{
		ServiceEndpointMessage message;
		if (!mIpAddressV4.isEmpty()) {
			message["ipAddressV4"_f] = mIpAddressV4.copyAsVector();
		}
		else {
			message["domain_name"_f] = mDomainName;
		}
		message["port"_f] = mPort;
		return message;
	}

	std::string ServiceEndpoint::getIpAddressV4String() const
	{
		std::string ipv4String;
		if (mIpAddressV4.size() == 4) {
			auto v = mIpAddressV4.copyAsVector();
			for (int i = 0; i < v.size(); i++) {
				if (i) {
					ipv4String += '.';
				}
				ipv4String += std::to_string(v[i]);
			}
		}
		else {
			throw GradidoNodeInvalidDataException("invalid ip4 address string in ServiceEndpoint");
		}
		return ipv4String;
	}

	std::string ServiceEndpoint::getConnectionString() const
	{
		std::string url;
		if (!mIpAddressV4.isEmpty()) {
			url = getIpAddressV4String();
		}
		else {
			url = mDomainName;
		}
		url += ':';
		url += std::to_string(mPort);
		return url;
	}
}