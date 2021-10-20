cmake_minimum_required(VERSION 3.18.2)
project(Gradido_Node C CXX)
SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY "bin" )

SET(CMAKE_CXX_FLAGS "-std=c++17" )
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_EXTENSIONS OFF)

include_directories(
	"dependencies" 
	"dependencies/jsonrpcpp/include"
	"dependencies/protobuf/src"
	"build/proto"
	"dependencies/rapidjson/include"
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

############################## find protobuf  #########################################
# prevent problems with two libs including googletest 
option(protobuf_BUILD_TESTS "Build tests" OFF)
add_subdirectory(dependencies/protobuf/cmake)

############################## parse protobuf files ###################################

FILE(TO_NATIVE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/src/proto GRADIDO_PROTO_MODEL_PATH)
FILE(TO_NATIVE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/build/proto PROTOBINDING_PATH)
file(MAKE_DIRECTORY ${PROTOBINDING_PATH})
file(MAKE_DIRECTORY ${PROTOBINDING_PATH}/gradido)

FILE(GLOB DATAMODEL_GRADIDO_PROTOS "${GRADIDO_PROTO_MODEL_PATH}/gradido/*.proto")

IF(WIN32)
	set(PROTOC_BIN_PATH "build/dependencies/protobuf/cmake/bin")
	find_program(PROTOBUF_PROTOC_EXECUTABLE protoc PATHS "${PROTOC_BIN_PATH}/Debug" )
	find_program(PROTOBUF_PROTOC_EXECUTABLE protoc PATHS "${PROTOC_BIN_PATH}/Release" )
ELSE()
	find_program(PROTOBUF_PROTOC_EXECUTABLE protoc PATHS "build/dependencies/protobuf/cmake/bin" )
ENDIF()
FILE(TO_NATIVE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/dependencies/protobuf/src GOOGLE_PROTOBUF_INCLUDES)
MESSAGE("protoc: ${PROTOBUF_PROTOC_EXECUTABLE} in build/dependencies/protobuf/bin/${CMAKE_BUILD_TYPE}")
FOREACH(proto ${DATAMODEL_GRADIDO_PROTOS})
   FILE(TO_NATIVE_PATH ${proto} proto_native)
   get_filename_component(proto_parsed ${proto} NAME_WLE)
   FILE(TO_NATIVE_PATH ${PROTOBINDING_PATH}/gradido/${proto_parsed}.pb.h proto_parsed_native)
   
   IF(${proto_native} IS_NEWER_THAN ${proto_parsed_native})
		EXECUTE_PROCESS(
			COMMAND 
			${PROTOBUF_PROTOC_EXECUTABLE} 
			--proto_path=${GRADIDO_PROTO_MODEL_PATH} 
			--cpp_out=${PROTOBINDING_PATH} 
			${proto_native}
			RESULT_VARIABLE rv
		)
		# Optional, but that can show the user if something have gone wrong with the proto generation 
		IF(${rv})
			MESSAGE("Generation of data model returned ${rv} for proto ${proto_native}")
		ELSE()
			MESSAGE("Parsed: src/proto/gradido/${proto_parsed}.proto")
		ENDIF()
   ENDIF()

ENDFOREACH(proto)



FILE(GLOB CONTAINER, "src/cpp/container/*.h" "src/cpp/container/*.cpp")
FILE(GLOB CONTROLLER "src/cpp/controller/*.cpp" "src/cpp/controller/*.h")
FILE(GLOB HTTPInterface "src/cpp/HTTPInterface/*.h" "src/cpp/HTTPInterface/*.cpp")
FILE(GLOB JSONRPCInterface "src/cpp/JSONRPCInterface/*.h" "src/cpp/JSONRPCInterface/*.cpp")
FILE(GLOB LIB_SRC "src/cpp/lib/*.h" "src/cpp/lib/*.cpp" "src/cpp/lib/*.c")
FILE(GLOB MODEL "src/cpp/model/*.h" "src/cpp/model/*.cpp")
FILE(GLOB MODEL_FILES "src/cpp/model/files/*.h" "src/cpp/model/files/*.cpp")
FILE(GLOB MODEL_TRANSACTIONS "src/cpp/model/transactions/*.h" "src/cpp/model/transactions/*.cpp")
FILE(GLOB PROTO_GRADIDO "build/proto/gradido/*.cc" "build/proto/gradido/*.h")
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


target_link_libraries(Gradido_Node leveldb libprotobuf ${CONAN_LIBS} )
if(WIN32)
TARGET_LINK_LIBRARIES(Gradido_Node optimized Shlwapi)
TARGET_LINK_LIBRARIES(Gradido_Node debug Shlwapi)
else()
target_link_libraries(Gradido_Node ${POCO_LIBS} -pthread)
endif()
#PocoNet PocoUtil PocoJSON PocoFoundation PocoData PocoNetSSL

# install 
if(UNIX)
install(TARGETS Gradido_Node RUNTIME DESTINATION /usr/local/bin)
endif(UNIX)

enable_testing()

#  ----------------------    Test -----------------------------------------

add_subdirectory("dependencies/googletest")

add_executable(Gradido_Node_Test ${LOCAL_SRCS} ${TEST} ${TEST_MODEL} ${TEST_MODEL_FILES} ${TEST_CONTROLLER})
target_compile_definitions(Gradido_Node_Test PUBLIC "_TEST_BUILD")
#add_test(main Gradido_Node_Test)

target_link_libraries(Gradido_Node_Test libprotobuf leveldb ${CONAN_LIBS})

if(WIN32)
	
else()
	target_link_libraries(Gradido_Node_Test ${POCO_LIBS} -pthread)
	#PocoNet PocoUtil PocoJSON PocoFoundation PocoData PocoNetSSL
endif()

target_link_libraries(Gradido_Node_Test gtest)

