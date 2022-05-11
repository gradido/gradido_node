cmake_minimum_required(VERSION 3.18.2)
project(GradidoNode C CXX)
SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY "bin" )

IF(WIN32)
	set(CMAKE_CXX_FLAGS         "/MP /EHsc /std:c++17")
ELSE()
	SET(CMAKE_CXX_FLAGS "-std=c++17" )
ENDIF()
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_EXTENSIONS OFF)

set(INSTALL_BINDIR "bin")
set(INSTALL_PLUGINDIR "bin")

include_directories(
	"dependencies" 
	"dependencies/jsonrpcpp/include"
	"dependencies/rapidjson/include"
	"dependencies/gradido_blockchain/include"
	"dependencies/gradido_blockchain/build"
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

IF(WIN32)
	include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
	conan_basic_setup()
ENDIF()

add_subdirectory("dependencies/jsonrpcpp")
option(LEVELDB_BUILD_TESTS "Build LevelDB's unit tests" OFF)
add_subdirectory("dependencies/leveldb")
#add_subdirectory("dependencies/googletest")


if(NOT BUILD_SHARED_LIBS)
	#add_subdirectory("dependencies/gradido_blockchain/dependencies/protobuf")
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

	set(POCO_LIBS Poco::Foundation Poco::Util Poco::Net Poco::NetSSL Poco::JWT)
ENDIF()
############################## find protobuf  #########################################
#add_subdirectory(dependencies/protobuf/cmake)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
# prevent problems with two libs including googletest 
option(protobuf_BUILD_TESTS "Build tests" OFF)

#message("binary dir: ${CMAKE_BINARY_DIR}")
add_subdirectory(dependencies/gradido_blockchain/dependencies/protobuf/cmake)

############################## add gradido blockchain  #########################################
add_subdirectory("dependencies/gradido_blockchain")

if(NOT BUILD_SHARED_LIBS)
	#option(protobuf_BUILD_TESTS "Build tests" OFF)
	#add_subdirectory("dependencies/gradido_blockchain/dependencies/protobuf/cmake")
endif()

add_executable(GradidoNode ${LOCAL_SRCS})
#SUBDIRS("src/test")


target_link_libraries(GradidoNode leveldb GradidoBlockchain)
if(NOT BUILD_SHARED_LIBS)
	target_link_libraries(GradidoNode libprotobuf)
endif()
if(WIN32)
	target_link_libraries(GradidoNode ${CONAN_LIBS})
	target_link_libraries(GradidoNode optimized Shlwapi)
	target_link_libraries(GradidoNode debug Shlwapi)
else()
	target_link_libraries(GradidoNode ${POCO_LIBS} sodium -pthread)
endif()
#PocoNet PocoUtil PocoJSON PocoFoundation PocoData PocoNetSSL

# install 
if(UNIX)
install(TARGETS GradidoNode RUNTIME DESTINATION /usr/local/bin)
endif(UNIX)

# add test
include(CTest)
add_subdirectory(test)


