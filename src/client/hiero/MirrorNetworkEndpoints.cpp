#include "MirrorNetworkEndpoints.h"


namespace client {
  namespace hiero {
      namespace MirrorNetworkEndpoints {
        const char* getByEndpointName(const char* endpointName) noexcept {
          if (strcmp(endpointName, "testnet") == 0) {
              return TESTNET;
          }
          else if (strcmp(endpointName, "previewnet") == 0) {
              return PREVIEWNET;
          }
          else if (strcmp(endpointName, "mainnet") == 0) {
              return MAINNET;
          }
          return nullptr;
        }
      }
  }
}
