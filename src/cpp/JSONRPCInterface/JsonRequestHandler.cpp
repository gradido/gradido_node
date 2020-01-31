#include "JsonRequestHandler.h"

#include "Poco/Net/HTTPServerRequest.h"
#include "Poco/Net/HTTPServerResponse.h"

//#include "Poco/URI.h"
//#include "Poco/DeflatingStream.h"

//#include "Poco/JSON/Parser.h"



//#include <exception>
#include <memory>


void JsonRequestHandler::handleRequest(Poco::Net::HTTPServerRequest& request, Poco::Net::HTTPServerResponse& response)
{

	response.setChunkedTransferEncoding(false);
	response.setContentType("application/json");
	//bool _compressResponse(request.hasToken("Accept-Encoding", "gzip"));
	//if (_compressResponse) response.set("Content-Encoding", "gzip");

	std::ostream& responseStream = response.send();
	//Poco::DeflatingOutputStream _gzipStream(_responseStream, Poco::DeflatingStreamBuf::STREAM_GZIP, 1);
	//std::ostream& responseStream = _compressResponse ? _gzipStream : _responseStream;

	auto method = request.getMethod();
	std::istream& request_stream = request.stream();
	//Poco::JSON::Object* json_result = nullptr;
	if (method == "POST" || method == "PUT") {
		// extract parameter from request
		//Poco::Dynamic::Var parsedResult = parseJsonWithErrorPrintFile(request_stream);
		//Poco::JSON::Parser jsonParser;

		Json request_json;

		request_stream >> request_json;

		try {
			jsonrpcpp::entity_ptr entity = jsonrpcpp::Parser::do_parse_json(request_json);
			Json result;
			if (entity->is_request()) {
				jsonrpcpp::request_ptr request = std::dynamic_pointer_cast<jsonrpcpp::Request>(entity);
				handle(*request, result);

				if (!result.is_null()) {
					jsonrpcpp::Response response(*request, result);
					responseStream << response.to_json();
					return;
				}
			}
		}
		catch (jsonrpcpp::InvalidRequestException& e) {
			Poco::Logger& errorLog(Poco::Logger::get("errorLog"));
			errorLog.error("error invalid request exception with: %s", e.what());
			//printf("request json: %s\n", request_json);
			responseStream << "{\"state\":\"error\", \"msg\": \"invalid request\"}";
		}
		catch (jsonrpcpp::RpcException& e) {
			Poco::Logger& errorLog(Poco::Logger::get("errorLog"));
			errorLog.error("error rpc exception with: %s", e.what());
			//printf("request json: %s\n", request_json);
			responseStream << "{\"state\":\"exception\"}";
		}

		responseStream << "{\"state\":\"error\", \"msg\":\"no result\"}";
		return;
	}
	//if (_compressResponse) _gzipStream.close();

	responseStream << "{\"state\":\"error\", \"msg\":\"no post or put request\"}";
}

