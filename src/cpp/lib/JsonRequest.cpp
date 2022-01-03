
#include "JsonRequest.h"

#include "Poco/Net/HTTPSClientSession.h"
#include "Poco/Net/HTTPRequest.h"
#include "Poco/Net/HTTPResponse.h"

#include "sodium.h"
#include "../ServerGlobals.h"
#include "../SingletonManager/MemoryManager.h"
#include "../SingletonManager/LoggerManager.h"
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

Document JsonRequest::GET(const char* methodName, ErrorList* errors/* = nullptr*/)
{
	static const char* functionName = "JsonRequest::GET";
	auto& log = LoggerManager::getInstance()->mErrorLogging;
	Document result;
	if (mServerHost.empty() || !mServerPort) {
		if (errors) {
			errors->addError(new Error(functionName, "server host or server port not given"));
		}
		else {
			log.error("[%s] server host or server port not given", functionName);
		}
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
		if (responseStringStream.str() == "Service Unavailable") {
			if (errors) {
				errors->addError(new Error(functionName, "Service Unavailable"));
			}
			
			return result;
		}
		//printf("response: %s\n", responseStringStream.str().data());
		std::string method_name(methodName);

		// extract parameter from request
		result.Parse(responseStringStream.str().data());
		
		if(result.HasParseError()) {
			if (errors) {
				errors->addError(new ParamError(functionName, "error parsing request answer", result.GetParseError()));
				errors->addError(new ParamError(functionName, "position of last parsing error", result.GetErrorOffset()));
				errors->addError(new ParamError(functionName, "result from server", responseStringStream.str().data()));
			}
			else {
				std::string functionNameString = functionName;
				log.error("[%s] error parsing request answer: %s on position: %d", functionNameString, result.GetParseError(), result.GetErrorOffset());
			}
			printf("response: %s\n", responseStringStream.str().data());
			
		}
	
	}
	catch (Poco::Exception& e) {
		if (errors) {
			errors->addError(new ParamError(functionName, "connect error to iota server", e.displayText().data()));
			errors->addError(new ParamError(functionName, "host", mServerHost));
			errors->addError(new ParamError(functionName, "port", mServerPort));
		}
		else {
			log.error("[%s] exception on connecting to iota server: %s (%s:%d)", e.displayText(), mServerHost, mServerPort);
		}
	}

	return result;
}

Document JsonRequest::PATCH(ErrorList* errors/* = nullptr*/)
{
	static const char* functionName = "JsonRequest::PATCH";
	auto& log = LoggerManager::getInstance()->mErrorLogging;
	Document result;
	if (mServerHost.empty() || !mServerPort) {
		if (errors) {
			errors->addError(new Error(functionName, "server host or server port not given"));
		}
		else {
			log.error("[%s] server host or server port not given", functionName);
		}
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
		Poco::Net::HTTPRequest request(Poco::Net::HTTPRequest::HTTP_PATCH, mUrlPath);

		request.setChunkedTransferEncoding(false);
		std::ostream& request_stream = clientSession->sendRequest(request);

		Poco::Net::HTTPResponse response;
		std::istream& response_stream = clientSession->receiveResponse(response);

		// debugging answer
		std::stringstream responseStringStream;
		for (std::string line; std::getline(response_stream, line); ) {
			responseStringStream << line << std::endl;
		}
				
		// extract parameter from request
		result.Parse(responseStringStream.str().data());

		if (result.HasParseError()) {
			if (errors) {
				errors->addError(new ParamError(functionName, "error parsing request answer", result.GetParseError()));
				errors->addError(new ParamError(functionName, "position of last parsing error", result.GetErrorOffset()));
				errors->addError(new ParamError(functionName, "result from server", responseStringStream.str().data()));
			}
			else {
				std::string functionNameString = functionName;
				log.error("[%s] error parsing request answer: %s on position: %d", functionNameString, result.GetParseError(), result.GetErrorOffset());
			}
			printf("response: %s\n", responseStringStream.str().data());

		}

	}
	catch (Poco::Exception& e) {
		if (errors) {
			errors->addError(new ParamError(functionName, "connect error to community server", e.displayText().data()));
			errors->addError(new ParamError(functionName, "host", mServerHost));
			errors->addError(new ParamError(functionName, "port", mServerPort));
		}
		else {
			log.error("[%s] exception on connecting to community server: %s (%s:%d)", e.displayText(), mServerHost, mServerPort);
		}
	}

	return result;
}




