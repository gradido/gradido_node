cmake_minimum_required(VERSION 3.18.2)
project(Gradido_Node C CXX)
SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY "bin" )

SET(CMAKE_CXX_FLAGS "-std=c++17" )
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_EXTENSIONS OFF)

include_directories(
	"dependencies" 
	"dependencies/jsonrpcpp/include"
	"build/proto"
	"dependencies/rapidjson/include"
	"dependencies/gradido_blockchain/include"
	"dependencies/gradido_blockchain/build/proto"
	#"dependencies/googletest/googletest/include"
	#"dependencies/leveldb/third_party/googletest/googlemock/include"
)

IF(UNIX)
	include_directories(
		"dependencies/poco/Crypto/include" 
		"dependencies/poco/Data/include" 
		"dependencies/poco/Util/include" 
		"dependencies/poco/Foundation/include"
		"dependencies/poco/JSON/include"
		"dependencies/poco/Net/include"
		"dependencies/poco/NetSSL_OpenSSL/include"
	)
ENDIF()

FILE(GLOB CONTAINER, "src/container/*.h" "src/container/*.cpp")
FILE(GLOB CONTROLLER "src/controller/*.cpp" "src/controller/*.h")
FILE(GLOB IOTA "src/iota/*.cpp" "src/iota/*.h")
FILE(GLOB HTTPInterface "src/HTTPInterface/*.h" "src/HTTPInterface/*.cpp")
FILE(GLOB JSONRPCInterface "src/JSONRPCInterface/*.h" "src/JSONRPCInterface/*.cpp")
FILE(GLOB LIB_SRC "src/lib/*.h" "src/lib/*.cpp" "src/lib/*.c")
FILE(GLOB MODEL "src/model/*.h" "src/model/*.cpp")
FILE(GLOB MODEL_FILES "src/model/files/*.h" "src/model/files/*.cpp")
FILE(GLOB SINGLETON_MANAGER "src/SingletonManager/*.h" "src/SingletonManager/*.cpp")
FILE(GLOB TASK "src/task/*.cpp" "src/task/*.h")
FILE(GLOB TCP_SERVER "src/TCPServer/*.cpp" "src/TCPServer/*.h")
FILE(GLOB VALIDATION "src/validation/*.h" "src/validation/*.cpp")
FILE(GLOB MAIN "src/*.cpp" "src/*.c"  "src/*.h")


SET(LOCAL_SRCS 
	${CONTAINER} ${CONTROLLER} ${IOTA}
	${HTTPInterface} ${JSONRPCInterface}
	${LIB_SRC}
	${MODEL} ${MODEL_FILES} 
	${SINGLETON_MANAGER}
	${TASK} ${TCP_SERVER}
	${VALIDATION}
	${MAIN}
)
#aux_source_directory("src/cpp" LOCAL_SRCS)

if(MSVC)
# src
source_group("container" FILES ${CONTAINER})
source_group("controller" FILES ${CONTROLLER})
source_group("iota" FILES ${IOTA})
source_group("HTTP-Interface" FILES ${HTTPInterface})
source_group("Json-rpc-Interface" FILES ${JSONRPCInterface})
source_group("lib" FILES ${LIB_SRC})
source_group("model\\files" FILES ${MODEL_FILES})
source_group("model" FILES ${MODEL})
source_group("SingletonManager" FILES ${SINGLETON_MANAGER})
source_group("task" FILES ${TASK})
source_group("tcp-server" FILES ${TCP_SERVER})
source_group("validation" FILES ${VALIDATION})
source_group("test\\lib" FILES ${TEST_LIB})
source_group("test\\model\\files" FILES ${TEST_MODEL_FILES})
source_group("test\\model" FILES ${TEST_MODEL})
source_group("test\\controller" FILES ${TEST_CONTROLLER})
source_group("test" FILES ${TEST})

endif(MSVC)

IF(WIN32)
	include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
	conan_basic_setup()
ENDIF()

add_subdirectory("dependencies/jsonrpcpp")
option(LEVELDB_BUILD_TESTS "Build LevelDB's unit tests" OFF)
add_subdirectory("dependencies/leveldb")
#add_subdirectory("dependencies/googletest")
add_subdirectory("dependencies/gradido_blockchain")

if(NOT BUILD_SHARED_LIBS)
	add_subdirectory("dependencies/gradido_blockchain/dependencies/protobuf")
endif()

############################## config and add poco ###################################
#SET(SOME_EXPAT_OPTION OFF CACHE BOOL "Use some expat option")
IF(UNIX)
	SET(ENABLE_MONGODB OFF CACHE BOOL "" FORCE) 
	SET(ENABLE_DATA_SQLITE OFF CACHE BOOL "" FORCE) 
	SET(ENABLE_REDIS OFF CACHE BOOL "" FORCE)
	SET(ENABLE_PAGECOMPILER_FILE2PAGE OFF CACHE BOOL "" FORCE)
	SET(ENABLE_PAGECOMPILER OFF CACHE BOOL "" FORCE)
	SET(ENABLE_JSON OFF CACHE BOOL "" FORCE)
	add_subdirectory("dependencies/poco")

	set(POCO_LIBS PocoFoundation PocoUtil PocoNet PocoNetSSL PocoData)
ENDIF()

add_executable(Gradido_Node ${LOCAL_SRCS})
#SUBDIRS("src/test")

if(WIN32)

#find_library(MYSQL_LIBRARIES_DEBUG  mariadbclient.lib PATHS "import/mariadb/lib/debug")

set(CMAKE_CXX_FLAGS         "/MP /EHsc")
#set(CMAKE_CXX_FLAGS_DEBUG   "-O0 -g3")
#set(CMAKE_CXX_FLAGS_RELEASE "-O3")

else()
# set vars for mariadb cmake files 
set(INSTALL_BINDIR "bin")
set(INSTALL_PLUGINDIR "bin")

endif(WIN32)


target_link_libraries(Gradido_Node leveldb GradidoBlockchain)
if(NOT BUILD_SHARED_LIBS)
	target_link_libraries(Gradido_Node libprotobuf)
endif()
if(WIN32)
	target_link_libraries(Gradido_Node ${CONAN_LIBS})
	target_link_libraries(Gradido_Node optimized Shlwapi)
	target_link_libraries(Gradido_Node debug Shlwapi)
else()
	target_link_libraries(Gradido_Node ${POCO_LIBS} sodium -pthread)
endif()
#PocoNet PocoUtil PocoJSON PocoFoundation PocoData PocoNetSSL

# install 
if(UNIX)
install(TARGETS Gradido_Node RUNTIME DESTINATION /usr/local/bin)
endif(UNIX)

# add test
include(CTest)
#add_subdirectory(test)
