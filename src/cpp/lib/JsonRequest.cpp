
#include "JsonRequest.h"

#include "Poco/Net/HTTPSClientSession.h"
#include "Poco/Net/HTTPRequest.h"
#include "Poco/Net/HTTPResponse.h"

#include "sodium.h"
#include "../ServerGlobals.h"
#include "../SingletonManager/MemoryManager.h"
#include "DataTypeConverter.h"

#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"

using namespace rapidjson;

JsonRequest::JsonRequest(const std::string& serverHost, int serverPort, const std::string& urlPath)
	: mServerHost(serverHost), mServerPort(serverPort), mUrlPath(urlPath)
{
	if (mServerHost.data()[mServerHost.size() - 1] == '/') {
		mServerHost = mServerHost.substr(0, mServerHost.size() - 1);
	}
}

JsonRequest::~JsonRequest()
{

}

using namespace rapidjson;

Document JsonRequest::GET(const char* methodName)
{
	static const char* functionName = "JsonRequest::GET";
	Document result;
	if (mServerHost.empty() || !mServerPort) {
		addError(new Error(functionName, "server host or server port not given"));
		return result;
	}
	try {

		Poco::SharedPtr<Poco::Net::HTTPClientSession> clientSession;
		if (mServerPort == 443) {
			clientSession = new Poco::Net::HTTPSClientSession(mServerHost, mServerPort, ServerGlobals::g_SSL_CLient_Context);
		}
		else {
			clientSession = new Poco::Net::HTTPClientSession(mServerHost, mServerPort);
		}
		Poco::Net::HTTPRequest request(Poco::Net::HTTPRequest::HTTP_GET, mUrlPath + methodName);

		request.setChunkedTransferEncoding(false);
		std::ostream& request_stream = clientSession->sendRequest(request);

		Poco::Net::HTTPResponse response;
		std::istream& response_stream = clientSession->receiveResponse(response);

		// debugging answer

		std::stringstream responseStringStream;
		for (std::string line; std::getline(response_stream, line); ) {
			responseStringStream << line << std::endl;
		}
		std::string method_name(methodName);

		// extract parameter from request
		result.Parse(responseStringStream.str().data());
		
		if(result.HasParseError()) {
			addError(new ParamError(functionName, "error parsing request answer", result.GetParseError()));
			addError(new ParamError(functionName, "position of last parsing error", result.GetErrorOffset()));
		}
	
	}
	catch (Poco::Exception& e) {
		addError(new ParamError(functionName, "connect error to php server", e.displayText().data()));
		addError(new ParamError(functionName, "host", mServerHost));
		addError(new ParamError(functionName, "port", mServerPort));
	}

	return result;
}




