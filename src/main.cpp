
#include "main.h"
#include "MainServer.h"
#ifdef _USING_IOTA
#include "MQTTAsync.h"
#endif //_USING_IOTA
#include "ServerGlobals.h"
#include "generated/version.h"

#include "gradido_blockchain/lib/Profiler.h"
#include "gradido_blockchain/version.h"

#include "sodium.h"
#include "loguru/loguru.hpp"

#include <exception>
#include <cstdlib>

#ifndef _TEST_BUILD

//! \return 0 if ok, -1 for error, 1 for abort
int parseParameter(int argc, char** argv)
{
	for (int i = 1; i < argc; i++) {
		if (strcmp(argv[i], "--version") == 0) {
			printf("%s\n", GRADIDO_NODE_VERSION);
			return 1;
		}
	}
	return 0;
};

int main(int argc, char** argv)
{
	auto parseResult = parseParameter(argc, argv);
	if (parseResult == 1) {
		return 0;
	}
	if (parseResult == -1) {
		return -1;
	}
	std::srand(std::time({}));
	// libsodium
	if (sodium_init() < 0) {
		/* panic! the library couldn't be initialized, it is not safe to use */
		printf("error initing sodium, early exit\n");
		return -1;
	}
#ifdef _USING_IOTA
	// mqtt
	MQTTAsync_init_options options = MQTTAsync_init_options_initializer;
	options.do_openssl_init = 0;
	MQTTAsync_global_init(&options);
#endif //_USING_IOTA

	// loguru 
	loguru::init(argc, argv);
	LOG_F(INFO, "Gradido Blockchain Library Version: %s", GRADIDO_BLOCKCHAIN_VERSION);
	LOG_F(INFO, "Gradido Node Version: %s", GRADIDO_NODE_VERSION);
	
	MainServer app;
	try {
		app.run();
	}
	catch (GradidoBlockchainException& ex) {
		LOG_F(ERROR, "Gradido Blockchain Exception while init: %s\n", ex.getFullString().data());
		return -1;
	}
	catch (std::exception& ex) {
		LOG_F(ERROR, "std exception while init: %s\n", ex.what());
		return -2;
	}
	ServerGlobals::clearMemory();
	return 0;
}
#endif
