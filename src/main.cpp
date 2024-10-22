
#include "main.h"
#include "MainServer.h"
#include "MQTTAsync.h"
#include "ServerGlobals.h"

#include "gradido_blockchain/lib/Profiler.h"

#include "sodium.h"
#include "loguru/loguru.hpp"

#include <exception>

#ifndef _TEST_BUILD

int main(int argc, char** argv)
{
	// libsodium
	if (sodium_init() < 0) {
		/* panic! the library couldn't be initialized, it is not safe to use */
		printf("error initing sodium, early exit\n");
		return -1;
	}
	// mqtt
	MQTTAsync_init_options options = MQTTAsync_init_options_initializer;
	options.do_openssl_init = 0;
	MQTTAsync_global_init(&options);

	// loguru 
	loguru::init(argc, argv);
	
	MainServer app;
	try {
		app.run();
	}
	catch (GradidoBlockchainException& ex) {
		LOG_F(FATAL, "Gradido Blockchain Exception while init: %s\n", ex.getFullString().data());
		return -1;
	}
	catch (std::exception& ex) {
		LOG_F(FATAL, "std exception while init: %s\n", ex.what());
		return -2;
	}
	ServerGlobals::clearMemory();
	return 0;
}
#endif
