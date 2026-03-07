

##### Run environment for tests #####
FROM alpine:3.22.2 AS test_run


##### BUILD-ENV #####
FROM denisgolius/zig:0.15.2 AS zig_build

ARG ZIG_TARGET=x86_64-linux-musl
#aarch64-linux-musl
ARG TARGET_ARCH=x86_64
# aarch64
ENV ZIG_TARGET=${ZIG_TARGET}
ENV TARGET_ARCH=${TARGET_ARCH}

RUN apk add --no-cache cmake make git binutils openssl-dev openssl-libs-static


##### BUILD debug #######
FROM zig_build AS debug_build

ARG ZIG_TARGET=x86_64-linux-musl
#aarch64-linux-musl
ARG TARGET_ARCH=x86_64

ARG GRADIDO_VERSION=0.0.0-dev
# aarch64
ENV ZIG_TARGET=${ZIG_TARGET}
ENV TARGET_ARCH=${TARGET_ARCH}
ENV GRADIDO_VERSION=${GRADIDO_VERSION}

# copy grpc-deps artifacts
COPY --from=gradido/grpc-deps:v1.74.1 /opt/${TARGET_ARCH}/local /usr/local

ENV DOCKER_WORKDIR="/code"
WORKDIR ${DOCKER_WORKDIR}

COPY . .

RUN mkdir build
WORKDIR ${DOCKER_WORKDIR}/build
RUN cmake .. \
    -DENABLE_TEST=On \
    -DENABLE_HTTPS=On \
    -DUSE_INSTALLED_SSL=On \
    -DCMAKE_EXE_LINKER_FLAGS="-static" \
    -DBUILD_SHARED_LIBS=Off \
    -DTARGET=${ZIG_TARGET} \
    -DGRPC_FETCH_CONTENT=Off
    # OpenSSL paths for CMake
    #-DOPENSSL_ROOT_DIR=/opt/${TARGET_ARCH}/usr \
    #-DOPENSSL_INCLUDE_DIR=/opt/${TARGET_ARCH}/usr/include \
    #-DOPENSSL_CRYPTO_LIBRARY=/opt/${TARGET_ARCH}/usr/lib/libcrypto.a \
    #-DOPENSSL_SSL_LIBRARY=/opt/${TARGET_ARCH}/usr/lib/libssl.a
RUN make -j$(nproc) GradidoNode

ENTRYPOINT []

##### BUILD release #######
FROM zig_build AS release_build

ARG ZIG_TARGET=x86_64-linux-musl
#aarch64-linux-musl
ARG TARGET_ARCH=x86_64

ARG GRADIDO_VERSION=0.0.0-dev
# aarch64
ENV ZIG_TARGET=${ZIG_TARGET}
ENV TARGET_ARCH=${TARGET_ARCH}
ENV GRADIDO_VERSION=${GRADIDO_VERSION}

# copy grpc-deps artifacts
COPY --from=gradido/grpc-deps:v1.74.1 /opt/${TARGET_ARCH}/local /usr/local

ENV DOCKER_WORKDIR="/code"
WORKDIR ${DOCKER_WORKDIR}

COPY . .

RUN mkdir build
WORKDIR ${DOCKER_WORKDIR}/build
RUN cmake .. \
    -DENABLE_TEST=Off \
    -DENABLE_HTTPS=On \
    -DUSE_INSTALLED_SSL=On \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_C_FLAGS_RELEASE="-O2 -DNDEBUG" \
    -DCMAKE_CXX_FLAGS_RELEASE="-O2 -DNDEBUG" \
    -DCMAKE_C_FLAGS="-g0" \
    -DCMAKE_CXX_FLAGS="-g0" \
    -DCMAKE_EXE_LINKER_FLAGS="-static" \
    -DBUILD_SHARED_LIBS=Off \
    -DTARGET=${ZIG_TARGET} \
    -DGRPC_FETCH_CONTENT=Off
    # OpenSSL paths for CMake
    #-DOPENSSL_ROOT_DIR=/opt/${TARGET_ARCH}/usr \
    #-DOPENSSL_INCLUDE_DIR=/opt/${TARGET_ARCH}/usr/include \
    #-DOPENSSL_CRYPTO_LIBRARY=/opt/${TARGET_ARCH}/usr/lib/libcrypto.a \
    #-DOPENSSL_SSL_LIBRARY=/opt/${TARGET_ARCH}/usr/lib/libssl.a
RUN make -j$(nproc) GradidoNode

ENTRYPOINT ["/bin/GradidoNode"]


##### run tests #######
FROM test_run AS test_build_run

COPY --from=debug_build /code/build/test/GradidoBlockchainTest ./

ENTRYPOINT []

