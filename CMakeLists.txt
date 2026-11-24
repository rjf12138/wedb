cmake_minimum_required(VERSION 3.10)

project(wedb VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_DEBUG_POSTFIX d)

# Compiler flags
set(CXX_FLAGS
    -O3
    -DCHECK_PTHREAD_RETURN_VALUE
    -D_FILE_OFFSET_BITS=64
    -march=native
    -rdynamic
    -O2 -Wall
    -DNDEBUG
)
string(REPLACE ";" " " CMAKE_CXX_FLAGS "${CXX_FLAGS}")

# Output directories
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/output/release/bin)
set(LIBRARY_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/output/release/lib)

# Source files
file(GLOB_RECURSE SRC_FILES
    ${CMAKE_SOURCE_DIR}/lsm/*.cc
    ${CMAKE_SOURCE_DIR}/lsm/*.cpp
    ${CMAKE_SOURCE_DIR}/lsm/*.cxx
    ${CMAKE_SOURCE_DIR}/lsm/*.c
)

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/extern_inc)
include_directories(${CMAKE_SOURCE_DIR}/lsm)

# Link directories
link_directories(${CMAKE_SOURCE_DIR}/lib/release)

# Build targets
add_executable(wedb
    ${CMAKE_SOURCE_DIR}/main/wedb_main.cc
    ${SRC_FILES}
)

add_executable(entry_test
    ${CMAKE_SOURCE_DIR}/tests/entry_test.cc
    ${SRC_FILES}
)

add_executable(bloom_test
    ${CMAKE_SOURCE_DIR}/tests/bloom_test.cc
    ${SRC_FILES}
)

add_executable(memtable_test
    ${CMAKE_SOURCE_DIR}/tests/memtable_test.cc
    ${SRC_FILES}
)

add_executable(wal_test
    ${CMAKE_SOURCE_DIR}/tests/wal_test.cc
    ${SRC_FILES}
)

add_executable(sstable_test
    ${CMAKE_SOURCE_DIR}/tests/sstable_test.cc
    ${SRC_FILES}
)

add_executable(db_test
    ${CMAKE_SOURCE_DIR}/tests/db_test.cc
    ${SRC_FILES}
)

# Include dirs for target wedb
target_include_directories(wedb PRIVATE ${CMAKE_SOURCE_DIR}/extern_inc)
target_include_directories(wedb PRIVATE ${CMAKE_SOURCE_DIR}/lsm)

# Include dirs for target entry_test
target_include_directories(entry_test PRIVATE ${CMAKE_SOURCE_DIR}/extern_inc)
target_include_directories(entry_test PRIVATE ${CMAKE_SOURCE_DIR}/lsm)

# Include dirs for target bloom_test
target_include_directories(bloom_test PRIVATE ${CMAKE_SOURCE_DIR}/extern_inc)
target_include_directories(bloom_test PRIVATE ${CMAKE_SOURCE_DIR}/lsm)

# Include dirs for target memtable_test
target_include_directories(memtable_test PRIVATE ${CMAKE_SOURCE_DIR}/extern_inc)
target_include_directories(memtable_test PRIVATE ${CMAKE_SOURCE_DIR}/lsm)

# Include dirs for target wal_test
target_include_directories(wal_test PRIVATE ${CMAKE_SOURCE_DIR}/extern_inc)
target_include_directories(wal_test PRIVATE ${CMAKE_SOURCE_DIR}/lsm)

# Include dirs for target sstable_test
target_include_directories(sstable_test PRIVATE ${CMAKE_SOURCE_DIR}/extern_inc)
target_include_directories(sstable_test PRIVATE ${CMAKE_SOURCE_DIR}/lsm)

# Include dirs for target db_test
target_include_directories(db_test PRIVATE ${CMAKE_SOURCE_DIR}/extern_inc)
target_include_directories(db_test PRIVATE ${CMAKE_SOURCE_DIR}/lsm)

# Legacy dependencies
target_link_libraries(wedb
    algorithm
    basic
    gtest
    pthread
)
target_link_libraries(entry_test
    algorithm
    basic
    gtest
    pthread
)
target_link_libraries(bloom_test
    algorithm
    basic
    gtest
    pthread
)
target_link_libraries(memtable_test
    algorithm
    basic
    gtest
    pthread
)
target_link_libraries(wal_test
    algorithm
    basic
    gtest
    pthread
)
target_link_libraries(sstable_test
    algorithm
    basic
    gtest
    pthread
)
target_link_libraries(db_test
    algorithm
    basic
    gtest
    pthread
)

