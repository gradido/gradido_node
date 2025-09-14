cmake_minimum_required(VERSION 3.18.2)
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
project(GradidoNode C CXX)
SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin )
message("runtime output directory: ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")

option(BUILD_SHARED_LIBS "Build using shared libraries" OFF)
IF(WIN32)
	set(CMAKE_CXX_FLAGS         "/MP /EHsc /std:c++20")
	set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
ELSE()
	SET(CMAKE_CXX_FLAGS "-std=c++20" )
ENDIF()
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
#set(CMAKE_CXX_EXTENSIONS OFF)

set(INSTALL_BINDIR "bin")
set(INSTALL_PLUGINDIR "bin")
# needed for protopuf
add_definitions(-DNOMINMAX)

include_directories(
	"dependencies" 
	"dependencies/jsonrpcpp/include"
	"dependencies/rapidjson/include"
	"dependencies/gradido_blockchain"
	"dependencies/gradido_blockchain/include"
	"dependencies/gradido_blockchain/build"
	"dependencies/gradido_blockchain/dependencies"
	"dependencies/gradido_blockchain/dependencies/date/include"
	"dependencies/gradido_blockchain/dependencies/magic_enum/include"
	"dependencies/gradido_blockchain/dependencies/protopuf/include"

	"dependencies/paho.mqtt.c/src"
	#"dependencies/googletest/googletest/include"
	#"dependencies/leveldb/third_party/googletest/googlemock/include"
)

FILE(GLOB BLOCKCHAIN "src/blockchain/*.h" "src/blockchain/*.cpp")
FILE(GLOB CACHE_SRC "src/cache/*.h" "src/cache/*.cpp")
FILE(GLOB CONTAINER, "src/container/*.h" "src/container/*.cpp")
FILE(GLOB CONTROLLER "src/controller/*.cpp" "src/controller/*.h")
FILE(GLOB HIERO "src/hiero/*.h" "src/hiero/*.cpp")
FILE(GLOB CLIENT "src/client/*.cpp" "src/client/*.h")
FILE(GLOB CLIENT_GRPC "src/client/grpc/*.cpp" "src/client/grpc/*.h")
FILE(GLOB CLIENT_HIERO "src/client/hiero/*.cpp" "src/client/hiero/*.h")
FILE(GLOB IOTA "src/iota/*.cpp" "src/iota/*.h")
FILE(GLOB SERVER "src/server/*.h" "src/server/*.cpp")
FILE(GLOB SERVER_JSON_RPC "src/server/json-rpc/*.h" "src/server/json-rpc/*.cpp")
FILE(GLOB LIB_SRC "src/lib/*.h" "src/lib/*.cpp" "src/lib/*.c")
FILE(GLOB MODEL "src/model/*.h" "src/model/*.cpp")
FILE(GLOB MODEL_APOLLO "src/model/Apollo/*.h" "src/model/Apollo/*.cpp")
FILE(GLOB MODEL_APOLLO_CREATE_TRANSACTION 
	"src/model/Apollo/createTransaction/*.h" 
	"src/model/Apollo/createTransaction/*.cpp"
)
FILE(GLOB MODEL_FILES "src/model/files/*.h" "src/model/files/*.cpp")
FILE(GLOB MODEL_JSON "src/model/json/*.h" "src/model/json/*.cpp")
FILE(GLOB SINGLETON_MANAGER "src/SingletonManager/*.h" "src/SingletonManager/*.cpp")
FILE(GLOB TASK "src/task/*.cpp" "src/task/*.h")
FILE(GLOB VIEW "src/view/*.h" "src/view/*.cpp")
FILE(GLOB MAIN "src/*.cpp" "src/*.c"  "src/*.h")


SET(LOCAL_SRCS 
	${BLOCKCHAIN}
	${CACHE_SRC}
	${CONTAINER} ${CONTROLLER} ${CLIENT} ${CLIENT_HIERO} ${CLIENT_GRPC}
	${HIERO}
	${SERVER} ${SERVER_JSON_RPC}
	${LIB_SRC}
	${MODEL}  ${MODEL_APOLLO} ${MODEL_APOLLO_CREATE_TRANSACTION} ${MODEL_FILES} ${MODEL_JSON}
	${SINGLETON_MANAGER}
	${TASK} ${VIEW}
	${MAIN}
)
#aux_source_directory("src/cpp" LOCAL_SRCS)

if(MSVC)
# src
source_group("blockchain" FILES ${BLOCKCHAIN})
source_group("cache" FILES ${CACHE_SRC})
source_group("container" FILES ${CONTAINER})
source_group("client\\hiero" FILES ${CLIENT_HIERO})
source_group("client\\grpc" FILES ${CLIENT_GRPC})
source_group("client" FILES ${CLIENT})
source_group("controller" FILES ${CONTROLLER})
source_group("hiero" FILES ${HIERO})
source_group("iota" FILES ${IOTA})
source_group("Json-rpc-Interface" FILES ${JSONRPCInterface})
source_group("lib" FILES ${LIB_SRC})
source_group("model\\Apollo\\createTransaction" FILES ${MODEL_APOLLO_CREATE_TRANSACTION})
source_group("model\\Apollo" FILES ${MODEL_APOLLO})
source_group("model\\files" FILES ${MODEL_FILES})
source_group("model\\json" FILES ${MODEL_JSON})
source_group("model" FILES ${MODEL})
source_group("server\\json-rpc" FILES ${SERVER_JSON_RPC})
source_group("server" FILES ${SERVER})
source_group("SingletonManager" FILES ${SINGLETON_MANAGER})
source_group("task" FILES ${TASK})

endif(MSVC)

############################## add dependencies  #########################################
set(ENABLE_HTTPS ON)
set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_SOURCE_DIR}/dependencies/gradido_blockchain/dependencies/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE FILEPATH "" FORCE)
set(VCPKG_TARGET_TRIPLET "x64-windows" CACHE STRING "vcpkg target triplet")
add_subdirectory("dependencies/gradido_blockchain")
# reuse cacert.pem from gradido_blockchain
add_compile_definitions(CACERT_PEM_PATH="${CACERT_PEM}")

add_subdirectory("dependencies/jsonrpcpp")
option(LEVELDB_BUILD_TESTS "Build LevelDB's unit tests" OFF)
add_subdirectory("dependencies/leveldb")
#add_subdirectory("dependencies/googletest")

if (MSVC)
	# Fetch Content Module
	include(FetchContent)

	# gRPC
	set(gRPC_SSL_PROVIDER "package" CACHE STRING "gRPC SSL provider" FORCE)
	message(STATUS "Make gRPC Available for Windows")
	FetchContent_Declare(
		grpc
		GIT_REPOSITORY https://github.com/grpc/grpc.git
		GIT_TAG        v1.74.1
	)
	FetchContent_MakeAvailable(grpc)

	# Since FetchContent uses add_subdirectory under the hood, we can use
	# the grpc targets directly from this build.
	# set(_PROTOBUF_LIBPROTOBUF libprotobuf)
	#set(_REFLECTION grpc++_reflection)
	#set(_PROTOBUF_PROTOC $<TARGET_FILE:protoc>)
	set(_GRPC_GRPCPP grpc++)
	#message("grpc++: ${_GRPC_GRPCPP}")
endif()


SET(GradidoBlockchain_BINARY_DIR ${GradidoNode_BINARY_DIR})
############################## mqtt client ############################################
#add_subdirectory("dependencies/paho.mqtt.c")

if(WIN32)
#find_package(libsodium REQUIRED)
include_directories(${libsodium_INCLUDE_DIR})
else()
#find_package(Sodium REQUIRED)
endif()

############################## find protobuf  #########################################
#add_subdirectory(dependencies/protobuf/cmake)

#set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
# prevent problems with two libs including googletest 
#option(protobuf_BUILD_TESTS "Build tests" OFF)

#message("binary dir: ${CMAKE_BINARY_DIR}")
#add_subdirectory(dependencies/gradido_blockchain/dependencies/protobuf)


if(NOT BUILD_SHARED_LIBS)
	#option(protobuf_BUILD_TESTS "Build tests" OFF)
	#add_subdirectory("dependencies/gradido_blockchain/dependencies/protobuf/cmake")
endif()

add_executable(GradidoNode ${LOCAL_SRCS})
#SUBDIRS("src/test")

target_link_libraries(GradidoNode leveldb GradidoBlockchain)# paho-mqtt3a)
if(WIN32)
	target_link_libraries(GradidoNode optimized Shlwapi)
	target_link_libraries(GradidoNode debug Shlwapi)
	#target_link_libraries(GradidoNode libsodium::libsodium)
	target_link_libraries(GradidoNode ${_GRPC_GRPCPP})
else()
  target_link_libraries(GradidoNode sodium)
endif()

IF(WIN32)
  add_custom_command(TARGET GradidoNode POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_RUNTIME_DLLS:GradidoNode> $<TARGET_FILE_DIR:GradidoNode>
    COMMAND_EXPAND_LISTS
  )
ENDIF()

# install 
if(UNIX)
install(TARGETS GradidoNode RUNTIME DESTINATION /usr/local/bin)
endif(UNIX)

# add test
#include(CTest)
#add_subdirectory(test)


