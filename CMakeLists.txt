cmake_minimum_required(VERSION 3.0)
project(Gradido_Node C CXX)
SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY "bin" )

add_definitions("-std=c++11")

include_directories(
	"dependencies" 
	"dependencies/iroha-ed25519/include"
	"dependencies/jsonrpcpp/include"
)


FILE(GLOB CONTAINER, "src/cpp/container/*.h" "src/cpp/container/*.cpp")
FILE(GLOB CONTROLLER "src/cpp/controller/*.cpp" "src/cpp/controller/*.h")
FILE(GLOB HTTPInterface "src/cpp/HTTPInterface/*.h" "src/cpp/HTTPInterface/*.cpp")
FILE(GLOB JSONRPCInterface "src/cpp/JSONRPCInterface/*.h" "src/cpp/JSONRPCInterface/*.cpp")
FILE(GLOB LIB_SRC "src/cpp/lib/*.h" "src/cpp/lib/*.cpp")
FILE(GLOB MODEL "src/cpp/model/*.h" "src/cpp/model/*.cpp")
FILE(GLOB MODEL_FILES "src/cpp/model/files/*.h" "src/cpp/files/table/*.cpp")
FILE(GLOB PROTO_GRADIDO "src/cpp/proto/gradido/*.cc" "src/cpp/proto/gradido/*.h")
FILE(GLOB SINGLETON_MANAGER "src/cpp/SingletonManager/*.h" "src/cpp/SingletonManager/*.cpp")
FILE(GLOB TASK "src/cpp/task/*.cpp" "src/cpp/task/*.h")
FILE(GLOB VALIDATION "src/cpp/validation/*.h" "src/cpp/validation/*.cpp")
FILE(GLOB MAIN "src/cpp/*.cpp" "src/cpp/*.c"  "src/cpp/*.h")

# used only for test project 
FILE(GLOB TEST "src/cpp/test/*.cpp" "src/cpp/test/*.h")

SET(LOCAL_SRCS 
	${CONTAINER} ${CONTROLLER}
	${HTTPInterface} ${JSONRPCInterface}
	${LIB_SRC}
	${MODEL} ${MODEL_FILES}
	${PROTO_GRADIDO}
	${SINGLETON_MANAGER}
	${TASK}
	${VALIDATION}
	${MAIN} ${TEST}
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
source_group("model" FILES ${MODEL})
source_group("proto" FILES ${PROTO_GRADIDO})
source_group("SingletonManager" FILES ${SINGLETON_MANAGER})
source_group("task" FILES ${TASK})
source_group("validation" FILES ${VALIDATION})
source_group("Test" FILES ${TEST})
endif(MSVC)

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup()

#add_subdirectory("dependencies/curl")
#add_subdirectory("dependencies/mariadb-connector-c")


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


target_link_libraries(Gradido_Node ${CONAN_LIBS} ${IROHA_ED25519})
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
