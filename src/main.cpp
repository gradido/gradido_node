
#include "main.h"
#include "MainServer.h"
#include "MQTTAsync.h"

#include <sodium.h>
#include <exception>
#include "gradido_blockchain/lib/Profiler.h"

#include "ServerGlobals.h"

#ifndef _TEST_BUILD


int main(int argc, char** argv)
{
	if (sodium_init() < 0) {
		/* panic! the library couldn't be initialized, it is not safe to use */
		printf("error initing sodium, early exit\n");
		return -1;
	}
	MQTTAsync_init_options options = MQTTAsync_init_options_initializer;
	options.do_openssl_init = 0;
	MQTTAsync_global_init(&options);
	
	MainServer app;
	int result = 0;
	try {
		result = app.run(argc, argv);
	}
	catch (Poco::Exception& ex) {
		printf("Poco Exception while init: %s\n", ex.displayText().data());
		return -1;
	}
	catch (GradidoBlockchainException& ex) {
		printf("Gradido Blockchain Exception while init: %s\n", ex.getFullString().data());
	}
	catch (std::exception& ex) {
		printf("std exception while init: %s\n", ex.what());
		return -2;
	}
	ServerGlobals::clearMemory();
	return result;
}
#endif
