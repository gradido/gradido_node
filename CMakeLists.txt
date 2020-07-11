cmake_minimum_required(VERSION 3.0)
project(Gradido_Node C CXX)
SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY "bin" )

add_definitions("-std=c++11")
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_EXTENSIONS OFF)

include_directories(
	"dependencies" 
	"dependencies/iroha-ed25519/include"
	"dependencies/jsonrpcpp/lib"
	#"dependencies/googletest/googletest/include"
	"dependencies/leveldb/third_party/googletest/googlemock/include"
)


FILE(GLOB CONTAINER, "src/cpp/container/*.h" "src/cpp/container/*.cpp")
FILE(GLOB CONTROLLER "src/cpp/controller/*.cpp" "src/cpp/controller/*.h")
FILE(GLOB HTTPInterface "src/cpp/HTTPInterface/*.h" "src/cpp/HTTPInterface/*.cpp")
FILE(GLOB JSONRPCInterface "src/cpp/JSONRPCInterface/*.h" "src/cpp/JSONRPCInterface/*.cpp")
FILE(GLOB LIB_SRC "src/cpp/lib/*.h" "src/cpp/lib/*.cpp" "src/cpp/lib/*.c")
FILE(GLOB MODEL "src/cpp/model/*.h" "src/cpp/model/*.cpp")
FILE(GLOB MODEL_FILES "src/cpp/model/files/*.h" "src/cpp/model/files/*.cpp")
FILE(GLOB MODEL_TRANSACTIONS "src/cpp/model/transactions/*.h" "src/cpp/model/transactions/*.cpp")
FILE(GLOB PROTO_GRADIDO "src/cpp/proto/gradido/*.cc" "src/cpp/proto/gradido/*.h")
FILE(GLOB SINGLETON_MANAGER "src/cpp/SingletonManager/*.h" "src/cpp/SingletonManager/*.cpp")
FILE(GLOB TASK "src/cpp/task/*.cpp" "src/cpp/task/*.h")
FILE(GLOB TCP_SERVER "src/cpp/TCPServer/*.cpp" "src/cpp/TCPServer/*.h")
FILE(GLOB VALIDATION "src/cpp/validation/*.h" "src/cpp/validation/*.cpp")
FILE(GLOB MAIN "src/cpp/*.cpp" "src/cpp/*.c"  "src/cpp/*.h")

# used only for test project 
FILE(GLOB TEST "src/cpp/test/*.cpp" "src/cpp/test/*.h")
FILE(GLOB TEST_MODEL "src/cpp/test/model/*.cpp" "src/cpp/test/model/*.h")
FILE(GLOB TEST_MODEL_FILES "src/cpp/test/model/files/*.cpp" "src/cpp/test/model/files/*.h")
FILE(GLOB TEST_CONTROLLER "src/cpp/test/controller/*.cpp" "src/cpp/test/controller/*.h")

SET(LOCAL_SRCS 
	${CONTAINER} ${CONTROLLER}
	${HTTPInterface} ${JSONRPCInterface}
	${LIB_SRC}
	${MODEL} ${MODEL_FILES} ${MODEL_TRANSACTIONS}
	${PROTO_GRADIDO}
	${SINGLETON_MANAGER}
	${TASK} ${TCP_SERVER}
	${VALIDATION}
	${MAIN}
)
aux_source_directory("src/cpp" LOCAL_SRCS)

if(MSVC)
# src
source_group("container" FILES ${CONTAINER})
source_group("controller" FILES ${CONTROLLER})
source_group("HTTP-Interface" FILES ${HTTPInterface})
source_group("Json-rpc-Interface" FILES ${JSONRPCInterface})
source_group("lib" FILES ${LIB_SRC})
source_group("model\\files" FILES ${MODEL_FILES})
source_group("model\\transactions" FILES ${MODEL_TRANSACTIONS})
source_group("model" FILES ${MODEL})
source_group("proto" FILES ${PROTO_GRADIDO})
source_group("SingletonManager" FILES ${SINGLETON_MANAGER})
source_group("task" FILES ${TASK})
source_group("tcp-server" FILES ${TCP_SERVER})
source_group("validation" FILES ${VALIDATION})
source_group("test\\model\\files" FILES ${TEST_MODEL_FILES})
source_group("test\\model" FILES ${TEST_MODEL})
source_group("test\\controller" FILES ${TEST_CONTROLLER})
source_group("test" FILES ${TEST})

endif(MSVC)

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup()

add_subdirectory("dependencies/jsonrpcpp")
add_subdirectory("dependencies/leveldb")
#add_subdirectory("dependencies/googletest")

add_executable(Gradido_Node ${LOCAL_SRCS})
#SUBDIRS("src/test")

if(WIN32)


#find_library(MYSQL_LIBRARIES_DEBUG  mariadbclient.lib PATHS "import/mariadb/lib/debug")

find_library(IROHA_ED25519 ed25519 PATHS "dependencies/iroha-ed25519/build/Debug" REQUIRED)


set(CMAKE_CXX_FLAGS         "/MP /EHsc")
#set(CMAKE_CXX_FLAGS_DEBUG   "-O0 -g3")
#set(CMAKE_CXX_FLAGS_RELEASE "-O3")

else(WIN32)
find_library(IROHA_ED25519 ed25519 PATHS "dependencies/iroha-ed25519/build" REQUIRED)
# set vars for mariadb cmake files 
set(INSTALL_BINDIR "bin")
set(INSTALL_PLUGINDIR "bin")

endif(WIN32)


target_link_libraries(Gradido_Node jsonrpcpp leveldb ${CONAN_LIBS} ${IROHA_ED25519})
if(WIN32)
TARGET_LINK_LIBRARIES(Gradido_Node optimized Shlwapi)
TARGET_LINK_LIBRARIES(Gradido_Node debug Shlwapi)
else(WIN32)
target_link_libraries(Gradido_Node PocoNet PocoUtil PocoJSON PocoFoundation PocoData PocoNetSSL protoc protobuf -pthread)
endif(WIN32)

# install 
if(UNIX)
install(TARGETS Gradido_Node RUNTIME DESTINATION /usr/local/bin)
endif(UNIX)

enable_testing()

#  ----------------------    Test -----------------------------------------

add_executable(Gradido_Node_Test ${LOCAL_SRCS} ${TEST} ${TEST_MODEL} ${TEST_MODEL_FILES} ${TEST_CONTROLLER})
target_compile_definitions(Gradido_Node_Test PUBLIC "_TEST_BUILD")
add_test(main Gradido_Node_Test)

target_link_libraries(Gradido_Node_Test gtest_main gmock jsonrpcpp leveldb ${CONAN_LIBS} ${IROHA_ED25519})

if(WIN32)
	
else(WIN32)
	target_link_libraries(Gradido_Node_Test PocoNet PocoUtil PocoJSON PocoFoundation PocoData PocoNetSSL protoc protobuf -pthread)
endif(WIN32)
