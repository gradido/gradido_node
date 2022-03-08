


# used only for test project 
FILE(GLOB TEST "src/cpp/test/*.cpp" "src/cpp/test/*.h")
FILE(GLOB TEST_LIB "src/cpp/test/lib/*.cpp" "src/cpp/test/lib/*.h")
FILE(GLOB TEST_MODEL "src/cpp/test/model/*.cpp" "src/cpp/test/model/*.h")
FILE(GLOB TEST_MODEL_FILES "src/cpp/test/model/files/*.cpp" "src/cpp/test/model/files/*.h")
FILE(GLOB TEST_CONTROLLER "src/cpp/test/controller/*.cpp" "src/cpp/test/controller/*.h")

enable_testing()

#  ----------------------    Test -----------------------------------------

add_subdirectory("dependencies/googletest")

add_executable(Gradido_Node_Test ${LOCAL_SRCS} ${TEST} ${TEST_LIB} ${TEST_MODEL} ${TEST_MODEL_FILES} ${TEST_CONTROLLER})
target_compile_definitions(Gradido_Node_Test PUBLIC "_TEST_BUILD")
#add_test(main Gradido_Node_Test)
if(WIN32)
	target_link_libraries(Gradido_Node_Test ${CONAN_LIBS})
else()
	target_link_libraries(Gradido_Node_Test ${CONAN_LIBS} sodium) 
endif()
target_link_libraries(Gradido_Node_Test libprotobuf leveldb)
target_link_libraries(Gradido_Node_Test ${POCO_LIBS} -pthread gtest) 
