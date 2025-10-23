get_filename_component(PROJECT_ROOT "${CMAKE_CURRENT_LIST_DIR}" ABSOLUTE)
message(STATUS "Project root dir: ${PROJECT_ROOT}")
include(${PROJECT_ROOT}/dependencies/gradido_blockchain/toolchain.cmake)
#set(CMAKE_C_COMPILER zig cc)
#set(CMAKE_CXX_COMPILER zig c++)
#set(CMAKE_C_COMPILER_TARGET ${TARGET})
#set(CMAKE_CXX_COMPILER_TARGET ${TARGET})

#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -target aarch64-linux-gnu")
#set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS}   -target aarch64-linux-gnu")

#set(CMAKE_SYSTEM_NAME Linux)
#set(CMAKE_SYSTEM_PROCESSOR aarch64)

# Cross-Compiler
#set(CMAKE_C_COMPILER   /usr/bin/aarch64-linux-gnu-gcc)
#set(CMAKE_CXX_COMPILER /usr/bin/aarch64-linux-gnu-g++)

# Root-Pfad für Header/Libs
#set(CMAKE_FIND_ROOT_PATH  /usr/aarch64-linux-gnu)

#set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
#set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
#set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)