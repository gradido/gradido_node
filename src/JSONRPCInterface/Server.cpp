// #include "cpp-httplib/httplib.h"

#include "Server.h"
#include "JsonRPCHandler.h"


#include "loguru/loguru.hpp"

Server::Server() : mServer(std::make_unique<httplib::Server>())
{

}

Server::~Server()
{
}

bool Server::run(const char* host/* = "0.0.0.0"*/, int port/* = 8340*/)
{
	mServer->set_file_request_handler([](const httplib::Request& req, httplib::Response& res) {
		res.status = httplib::StatusCode::BadRequest_400;
		res.set_content("File not Found", "text/plain");
	});
	mServer->set_logger([](const httplib::Request& req, const httplib::Response& res) {
		printf("request: %s, response: %s\n", req.body.data(), res.body.data());
	});
	mServer->set_error_handler([](const auto& req, auto& res) {
		LOG_F(ERROR, "error response for request: %s", req.body.data());
	});
	// json rpc server
	mServer->Get("/api", [](const httplib::Request& req, httplib::Response& res) {
		JsonRPCHandler handler;
		handler.handleRequest(req, res, MethodType::GET);
	});
	mServer->Post("/api", [](const httplib::Request& req, httplib::Response& res) {
		JsonRPCHandler handler;
		handler.handleRequest(req, res, MethodType::POST);
	});
	mServer->Put("/api", [](const httplib::Request& req, httplib::Response& res) {
		JsonRPCHandler handler;
		handler.handleRequest(req, res, MethodType::PUT);
	});
	mServer->Options("/api", [](const httplib::Request& req, httplib::Response& res) {
		JsonRPCHandler handler;
		handler.handleRequest(req, res, MethodType::OPTIONS);
	});
	mServer->listen(host, port);
	return true;
}