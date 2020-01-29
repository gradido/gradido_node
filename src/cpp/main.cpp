#include "mainServer.h"

#include "proto/gradido/TransactionBody.pb.h"
#include <sodium.h>
#include "lib/Profiler.h"

#ifndef _TEST_BUILD


int main(int argc, char** argv)
{
	GOOGLE_PROTOBUF_VERIFY_VERSION;
	if (sodium_init() < 0) {
		/* panic! the library couldn't be initialized, it is not safe to use */
		printf("error initing sodium, early exit\n");
		return -1;
	}


	MainServer app;
	return app.run(argc, argv);
}
#endif