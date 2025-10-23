##### Run environment for tests #####
FROM alpine:3.22.2 AS test_run


##### BUILD-ENV #####
FROM denisgolius/zig:0.15.2 AS zig_build

RUN apk add --no-cache cmake make git binutils openssl-dev openssl-libs-static

##### BUILD debug #######
FROM zig_build AS debug_build

ENV DOCKER_WORKDIR="/code"
WORKDIR ${DOCKER_WORKDIR}

COPY . .

RUN mkdir build
WORKDIR ${DOCKER_WORKDIR}/build
RUN cmake .. -DENABLE_TEST=On -DENABLE_HTTPS=On -DUSE_INSTALLED_SSL=On -DCMAKE_EXE_LINKER_FLAGS="-static" -DBUILD_SHARED_LIBS=Off
RUN make -j$(nproc) GradidoNode

ENTRYPOINT []

##### run tests #######
FROM test_run AS test_build_run

COPY --from=debug_build /code/build/test/GradidoBlockchainTest ./

ENTRYPOINT []

##### BUILD release #######
FROM zig_build AS release_build
ENV DOCKER_WORKDIR="/code"
WORKDIR ${DOCKER_WORKDIR}

COPY . .
RUN mkdir build
WORKDIR ${DOCKER_WORKDIR}/build
RUN cmake -DCMAKE_BUILD_TYPE=Release ..
RUN make -j$(nproc) GradidoBlockchain
