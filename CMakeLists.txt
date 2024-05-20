cmake_minimum_required(VERSION 3.18.2)
project(GradidoNode C CXX)
SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin )
message("runtime output directory: ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")

option(BUILD_SHARED_LIBS "Build using shared libraries" OFF)
IF(WIN32)
	set(CMAKE_CXX_FLAGS         "/MP /EHsc /std:c++17")
	set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
ELSE()
	SET(CMAKE_CXX_FLAGS "-std=c++17" )
ENDIF()
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
#set(CMAKE_CXX_EXTENSIONS OFF)

set(INSTALL_BINDIR "bin")
set(INSTALL_PLUGINDIR "bin")

include_directories(
	"dependencies" 
	"dependencies/jsonrpcpp/include"
	"dependencies/rapidjson/include"
	"dependencies/gradido_blockchain/include"
	"dependencies/gradido_blockchain/build"
	"dependencies/paho.mqtt.c/src"
	"dependencies/iota-pow-in-c/src/"
	#"dependencies/googletest/googletest/include"
	#"dependencies/leveldb/third_party/googletest/googlemock/include"
)

IF(UNIX)
	include_directories(
		"dependencies/poco/Crypto/include" 
		"dependencies/poco/Util/include" 
		"dependencies/poco/Foundation/include"
		"dependencies/poco/Net/include"
		"dependencies/poco/NetSSL_OpenSSL/include"
		"dependencies/poco/JWT/include"
	)
ENDIF()

FILE(GLOB CONTAINER, "src/container/*.h" "src/container/*.cpp")
FILE(GLOB CONTROLLER "src/controller/*.cpp" "src/controller/*.h")
FILE(GLOB CLIENT "src/client/*.cpp" "src/client/*.h")
FILE(GLOB IOTA "src/iota/*.cpp" "src/iota/*.h")
FILE(GLOB HTTPInterface "src/HTTPInterface/*.h" "src/HTTPInterface/*.cpp")
FILE(GLOB JSONRPCInterface "src/JSONRPCInterface/*.h" "src/JSONRPCInterface/*.cpp")
FILE(GLOB LIB_SRC "src/lib/*.h" "src/lib/*.cpp" "src/lib/*.c")
FILE(GLOB MODEL "src/model/*.h" "src/model/*.cpp")
FILE(GLOB MODEL_FILES "src/model/files/*.h" "src/model/files/*.cpp")
FILE(GLOB MODEL_APOLLO "src/model/Apollo/*.h" "src/model/Apollo/*.cpp")
FILE(GLOB SINGLETON_MANAGER "src/SingletonManager/*.h" "src/SingletonManager/*.cpp")
FILE(GLOB TASK "src/task/*.cpp" "src/task/*.h")
FILE(GLOB VIEW "src/view/*.h" "src/view/*.cpp")
FILE(GLOB MAIN "src/*.cpp" "src/*.c"  "src/*.h")


SET(LOCAL_SRCS 
	${CONTAINER} ${CONTROLLER} ${CLIENT} ${IOTA}
	${HTTPInterface} ${JSONRPCInterface}
	${LIB_SRC}
	${MODEL} ${MODEL_FILES} ${MODEL_APOLLO}
	${SINGLETON_MANAGER}
	${TASK} ${VIEW}
	${MAIN}
)
#aux_source_directory("src/cpp" LOCAL_SRCS)

if(MSVC)
# src
source_group("container" FILES ${CONTAINER})
source_group("client" FILES ${CLIENT})
source_group("controller" FILES ${CONTROLLER})
source_group("iota" FILES ${IOTA})
source_group("HTTP-Interface" FILES ${HTTPInterface})
source_group("Json-rpc-Interface" FILES ${JSONRPCInterface})
source_group("lib" FILES ${LIB_SRC})
source_group("model\\files" FILES ${MODEL_FILES})
source_group("model\\Apollo" FILES ${MODEL_APOLLO})
source_group("model" FILES ${MODEL})
source_group("SingletonManager" FILES ${SINGLETON_MANAGER})
source_group("task" FILES ${TASK})

endif(MSVC)

############################## add dependencies  #########################################

add_subdirectory("dependencies/gradido_blockchain")
SET(GradidoBlockchain_BINARY_DIR ${GradidoNode_BINARY_DIR})

add_subdirectory("dependencies/jsonrpcpp")
option(LEVELDB_BUILD_TESTS "Build LevelDB's unit tests" OFF)
add_subdirectory("dependencies/leveldb")
#add_subdirectory("dependencies/googletest")


############################## config and add poco ###################################
#SET(SOME_EXPAT_OPTION OFF CACHE BOOL "Use some expat option")
IF(UNIX)
	SET(ENABLE_MONGODB OFF CACHE BOOL "" FORCE) 
	SET(ENABLE_DATA_SQLITE OFF CACHE BOOL "" FORCE) 
	SET(ENABLE_REDIS OFF CACHE BOOL "" FORCE)
	SET(ENABLE_PAGECOMPILER_FILE2PAGE OFF CACHE BOOL "" FORCE)
	SET(ENABLE_PAGECOMPILER OFF CACHE BOOL "" FORCE)
	SET(ENABLE_JSON OFF CACHE BOOL "" FORCE)
	add_subdirectory("dependencies/gradido_blockchain/dependencies/poco")

	set(POCO_LIBS Poco::Foundation Poco::Util Poco::Net Poco::NetSSL Poco::JWT)
ENDIF()

############################## mqtt client ############################################
add_subdirectory("dependencies/paho.mqtt.c")

if(WIN32)
find_package(poco REQUIRED)
find_package(mpfr REQUIRED)
find_package(libsodium REQUIRED)
include_directories(
		${mpfr_INCLUDE_DIRS}
		${gmp_INCLUDE_DIRS}
		${libsodium_INCLUDE_DIR}
		${OPENSSL_INCLUDE_DIR}
		${Poco_INCLUDE_DIRS}
	)
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

target_link_libraries(GradidoNode leveldb GradidoBlockchain paho-mqtt3a)
if(NOT BUILD_SHARED_LIBS)
	target_link_libraries(GradidoNode libprotobuf)
endif()
if(WIN32)
	target_link_libraries(GradidoNode ${Poco_LIBRARIES})
	target_link_libraries(GradidoNode optimized Shlwapi)
	target_link_libraries(GradidoNode debug Shlwapi)
else()
	target_link_libraries(GradidoNode ${POCO_LIBS} sodium -pthread)
endif()
#PocoNet PocoUtil PocoJSON PocoFoundation PocoData PocoNetSSL

IF(WIN32)
  add_custom_command(TARGET GradidoNode POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_RUNTIME_DLLS:GradidoNode> $<TARGET_FILE_DIR:GradidoNode>
	#COMMAND ${CMAKE_COMMAND} -E copy_if_different
	 # "${PROJECT_BINARY_DIR}/dependencies/gradido_blockchain/dependencies/ed25519_bip32_c_interface/x86_64-pc-windows-msvc/${CMAKE_BUILD_TYPE}/ed25519_bip32_c_interface.dll"
	  #"$<TARGET_FILE_DIR:GradidoNode>"
	#COMMAND ${CMAKE_COMMAND} -E copy_if_different
	#  "${PROJECT_BINARY_DIR}/dependencies/gradido_blockchain/iota_rust_clib/x86_64-pc-windows-msvc/${CMAKE_BUILD_TYPE}/iota_rust_clib.dll"
	 # "$<TARGET_FILE_DIR:GradidoNode>"
	#COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_RUNTIME_DLLS:GradidoNode> $<TARGET_FILE_DIR:GradidoNode>
	#IOTA_RUST_C_LIB
	#ED25519_BIP32_RUST_C_LIB
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


