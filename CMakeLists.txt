cmake_minimum_required(VERSION 3.10)

# 设置项目名称
project(wedb)

set(CMAKE_DEBUG_POSTFIX d)

set(CXX_FLAGS
    -g
    # -DVALGRIND
    -DCHECK_PTHREAD_RETURN_VALUE
    -D_FILE_OFFSET_BITS=64
    -march=native
    # -MMD
    -rdynamic
    )
string(REPLACE ";" " " CMAKE_CXX_FLAGS "${CXX_FLAGS}")

# 设置生成文件输出路径
set(EXECUTABLE_OUTPUT_PATH, ./output/debug/bin)
add_executable(wedb ./main/FSetNode_Test.cc)

target_include_directories(wedb PUBLIC ./inc/)
target_include_directories(wedb PRIVATE ./extern_inc/)
target_include_directories(wedb PRIVATE struct)

target_link_directories(wedb PRIVATE ./lib/debug/)

target_link_libraries(wedb PRIVATE algorithmd)
target_link_libraries(wedb PRIVATE systemd)
target_link_libraries(wedb PRIVATE gtest)
target_link_libraries(wedb PRIVATE pthread)
target_link_libraries(wedb PRIVATE basicd)
include(src/CMakeLists.txt)
include(struct/CMakeLists.txt)

