#include "CertificateVerifier.h"
#include <openssl/evp.h>
#include <openssl/sha.h>

#include "loguru/loguru.hpp"

using namespace ::grpc;

namespace client {
	namespace grpc {

		CertificateVerifier::CertificateVerifier(memory::ConstBlockPtr certificateHash)
			: mExpectedHash(certificateHash)
		{

		}

        /**
         * The verification logic that will be performed after the TLS handshake completes.
         *
         * @param request     The verification information associated with this request.
         * @param callback    Callback for asynchronous requests. This is unused since the SDK does all requests
         *                    synchronously.
         * @param sync_status Pointer to the gRPC status that should be updated to indicate the success or the failure of the
         *                     check.
         * @return \c TRUE because all checks are done synchronously.
         */
        bool CertificateVerifier::Verify(experimental::TlsCustomVerificationCheckRequest* request,
            std::function<void(Status)> callback,
            Status* sync_status)
        {
            auto certChain = request->peer_cert_full_chain();
            if (certChain.empty()) {
                *sync_status = Status(StatusCode::UNAUTHENTICATED, "Empty peer cert chain");
                return true;
            }
            
            memory::Block hash(SHA384_DIGEST_LENGTH);
            SHA384(reinterpret_cast<const unsigned char*>(certChain.data()), certChain.size(), hash.data());

            // on failure
            if (mExpectedHash->isTheSame(hash)) {
                *sync_status = Status(StatusCode::OK, "Certificate chain hash matches expected");
            }
            else {
                *sync_status = Status(
                    StatusCode::UNAUTHENTICATED,
                    "Hash of node certificate chain doesn't match hash contained in address book"
                );
                LOG_F(ERROR, "expectedHash: %s, certchain hash: %s", mExpectedHash->convertToHex().data(), hash.convertToHex().data());
            }
            
            return true;
        }
	}
}