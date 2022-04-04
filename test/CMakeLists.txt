cmake_minimum_required(VERSION 3.18.2)
project(Gradido_Node_Test C CXX)
SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin" )

include_directories(
	"../dependencies" 
	"../dependencies/rapidjson/include"
	"../dependencies/gradido_blockchain/include"
	"../dependencies/gradido_blockchain/build"
	"../build/dependencies/gradido_blockchain/include"
)

# used only for test project 
FILE(GLOB TEST "*.cpp" "*.h")
FILE(GLOB TEST_LIB "lib/*.cpp" "lib/*.h")
FILE(GLOB TEST_MODEL "model/*.cpp" "model/*.h")
FILE(GLOB TEST_MODEL_FILES "model/files/*.cpp" "model/files/*.h")
FILE(GLOB TEST_CONTROLLER "controller/*.cpp" "controller/*.h")

enable_testing()

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

SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin" )


add_executable(Gradido_Node_Test ${LOCAL_SRCS} ${TEST} ${TEST_LIB} ${TEST_MODEL} ${TEST_MODEL_FILES} ${TEST_CONTROLLER})
target_compile_definitions(Gradido_Node_Test PUBLIC "_TEST_BUILD")
#add_test(main Gradido_Node_Test)
if(WIN32)
	target_link_libraries(Gradido_Node_Test ${CONAN_LIBS})
else()
	target_link_libraries(Gradido_Node_Test ${CONAN_LIBS} sodium) 
endif()
target_link_libraries(Gradido_Node_Test libprotobuf leveldb GradidoBlockchain)
target_link_libraries(Gradido_Node_Test ${POCO_LIBS} -pthread gtest) 
#add_test(NAME Gradido_Node_Test
 #        COMMAND Gradido_Node_Test)
include(GoogleTest)
gtest_discover_tests(Gradido_Node_Test)