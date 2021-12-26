cmake_minimum_required(VERSION 3.10)

# 设置项目名称
project(wedb)

set(CMAKE_DEBUG_POSTFIX d)

# 指定c++标准: 默认是 c++11
add_library(wedb_compiler_flags INTERFACE)
target_compile_features(wedb_compiler_flags INTERFACE cxx_std_11)
# 设置 c++ 警告标志
set(gcc_like_cxx "$<COMPILE_LANG_AND_ID:CXX,ARMClang,AppleClang,Clang,GNU>")
set(msvc_cxx "$<COMPILE_LANG_AND_ID:CXX,MSVC>")

target_compile_options(wedb_compiler_flags INTERFACE
		$<${gcc_like_cxx}:$<BUILD_INTERFACE:-Wall;-Wextra;-Wshadow;-Wformat=2;-Wunused>>
		$<${msvc_cxx}:$<BUILD_INTERFACE:-W3>>)

# 设置生成文件输出路径
set(EXECUTABLE_OUTPUT_PATH, ./output/debug/bin)
add_executable(wedb ./main/DSN_hash_table_test.cc)

target_include_directories(wedb PUBLIC ./inc/)
target_include_directories(wedb PRIVATE ./extern_inc/)
target_include_directories(wedb PRIVATE struct)

target_link_directories(wedb PRIVATE ./lib/debug/)

target_link_libraries(wedb PRIVATE systemd)
target_link_libraries(wedb PRIVATE gtest)
target_link_libraries(wedb PRIVATE pthread)
target_link_libraries(wedb PRIVATE basicd)
include(src/CMakeLists.txt)
include(struct/CMakeLists.txt)

