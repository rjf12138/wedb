cmake_minimum_required(VERSION 3.10)

project(wedb VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_DEBUG_POSTFIX d)

# Compiler flags
set(CXX_FLAGS
    -g
    -ggdb
    -DCHECK_PTHREAD_RETURN_VALUE
    -D_FILE_OFFSET_BITS=64
    -march=native
    -rdynamic
    -O0 -Wall
    -DDEBUG
)
string(REPLACE ";" " " CMAKE_CXX_FLAGS "${CXX_FLAGS}")

# Output directories
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/output/debug/bin)
set(LIBRARY_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/output/debug/lib)

# Source files
file(GLOB_RECURSE SRC_FILES
    ${CMAKE_SOURCE_DIR}/src/*.cc
    ${CMAKE_SOURCE_DIR}/src/*.cpp
    ${CMAKE_SOURCE_DIR}/src/*.cxx
    ${CMAKE_SOURCE_DIR}/src/*.c
    ${CMAKE_SOURCE_DIR}/lsm/*.cc
    ${CMAKE_SOURCE_DIR}/lsm/*.cpp
    ${CMAKE_SOURCE_DIR}/lsm/*.cxx
    ${CMAKE_SOURCE_DIR}/lsm/*.c
)

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/inc)
include_directories(${CMAKE_SOURCE_DIR}/extern_inc)
include_directories(${CMAKE_SOURCE_DIR}/src)
include_directories(${CMAKE_SOURCE_DIR}/lsm)

# Link directories
link_directories(${CMAKE_SOURCE_DIR}/lib/debug)

# Build targets
add_executable(wedb
    ${CMAKE_SOURCE_DIR}/main/utils_test.cc
    ${SRC_FILES}
)

# Include dirs for target wedb
target_include_directories(wedb PRIVATE ${CMAKE_SOURCE_DIR}/inc)
target_include_directories(wedb PRIVATE ${CMAKE_SOURCE_DIR}/extern_inc)
target_include_directories(wedb PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_include_directories(wedb PRIVATE ${CMAKE_SOURCE_DIR}/lsm)

# Legacy dependencies
target_link_libraries(wedb
    algorithm
    basic
    gtest
    pthread
)

