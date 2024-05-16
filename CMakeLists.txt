cmake_minimum_required(VERSION 3.18.2)
project(GradidoNode C CXX)
SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin )
message("runtime output directory: ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")

IF(WIN32)
	set(CMAKE_CXX_FLAGS         "/MP /EHsc /std:c++17")
ELSE()
	SET(CMAKE_CXX_FLAGS "-std=c++20" )
ENDIF()
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_EXTENSIONS OFF)

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

IF(WIN32)
	#include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
	#conan_basic_setup()
	set(libsodium_DIR ${CMAKE_BINARY_DIR})
	find_package(libsodium CONFIG REQUIRED)
	set(gmp_DIR ${CMAKE_BINARY_DIR})
	set(MPFR_DIR ${CMAKE_BINARY_DIR})
	find_package(MPFR REQUIRED)	
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
	add_subdirectory("dependencies/gradido_blockchain/dependencies/poco")

	set(POCO_LIBS Poco::Foundation Poco::Util Poco::Net Poco::NetSSL Poco::JWT)
ENDIF()

############################## mqtt client ############################################
add_subdirectory("dependencies/paho.mqtt.c")

############################## iota pow ###############################################
# Path to the source files
set(IOTA_POW_SRC_DIR "dependencies/iota-pow-in-c/src")

# List of file names without file extension
set(IOTA_POW_FILENAMES
    constants
    curl
		trinary
)

# set correct preprocessor flag
include(CheckCCompilerFlag)
CHECK_C_COMPILER_FLAG("-mavx" COMPILER_SUPPORTS_AVX)
if(COMPILER_SUPPORTS_AVX)
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mavx")
    add_definitions(-DAVX)
		list(APPEND IOTA_POW_FILENAMES pow_avx)
else()
    # Wenn AVX nicht unterstützt wird, überprüfe, ob SSE unterstützt wird
    CHECK_C_COMPILER_FLAG("-msse" COMPILER_SUPPORTS_SSE)
    if(COMPILER_SUPPORTS_SSE)
		    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -msse")
        add_definitions(-DSSE)
				list(APPEND IOTA_POW_FILENAMES pow_sse)
		else()
				add_definitions(-DC)
				list(APPEND IOTA_POW_FILENAMES pow_c)
    endif()
endif()


# Create a list of C files
foreach(filename ${IOTA_POW_FILENAMES})
    list(APPEND IOTA_POW_FILES ${IOTA_POW_SRC_DIR}/${filename}.c)
endforeach()

# Add corresponding header files
foreach(filename ${IOTA_POW_FILENAMES})
    list(APPEND IOTA_POW_FILES ${IOTA_POW_SRC_DIR}/${filename}.h)
endforeach()

# Add the list of files to the library
add_library(iotaPOW ${IOTA_POW_FILES})
############################## find protobuf  #########################################
#add_subdirectory(dependencies/protobuf/cmake)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
# prevent problems with two libs including googletest 
option(protobuf_BUILD_TESTS "Build tests" OFF)

#message("binary dir: ${CMAKE_BINARY_DIR}")
add_subdirectory(dependencies/gradido_blockchain/dependencies/protobuf)

############################## add gradido blockchain  #########################################
add_subdirectory("dependencies/gradido_blockchain")
SET(GradidoBlockchain_BINARY_DIR ${GradidoNode_BINARY_DIR})

if(NOT BUILD_SHARED_LIBS)
	#option(protobuf_BUILD_TESTS "Build tests" OFF)
	#add_subdirectory("dependencies/gradido_blockchain/dependencies/protobuf/cmake")
endif()

add_executable(GradidoNode ${LOCAL_SRCS})
#SUBDIRS("src/test")


target_link_libraries(GradidoNode leveldb GradidoBlockchain paho-mqtt3a iotaPOW)
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


