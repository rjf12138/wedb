# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.22

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:

#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:

# Disable VCS-based implicit rules.
% : %,v

# Disable VCS-based implicit rules.
% : RCS/%

# Disable VCS-based implicit rules.
% : RCS/%,v

# Disable VCS-based implicit rules.
% : SCCS/s.%

# Disable VCS-based implicit rules.
% : s.%

.SUFFIXES: .hpux_make_needs_suffix_list

# Command-line flag to silence nested $(MAKE).
$(VERBOSE)MAKESILENT = -s

#Suppress display of executed commands.
$(VERBOSE).SILENT:

# A target that is always out of date.
cmake_force:
.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /usr/bin/cmake

# The command to remove a file.
RM = /usr/bin/cmake -E rm -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/ruanjian/workspace/project_v/wedb

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/ruanjian/workspace/project_v/wedb/build

# Include any dependencies generated for this target.
include CMakeFiles/wedb.dir/depend.make
# Include any dependencies generated by the compiler for this target.
include CMakeFiles/wedb.dir/compiler_depend.make

# Include the progress variables for this target.
include CMakeFiles/wedb.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/wedb.dir/flags.make

CMakeFiles/wedb.dir/main/utils_test.cc.o: CMakeFiles/wedb.dir/flags.make
CMakeFiles/wedb.dir/main/utils_test.cc.o: ../main/utils_test.cc
CMakeFiles/wedb.dir/main/utils_test.cc.o: CMakeFiles/wedb.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/ruanjian/workspace/project_v/wedb/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object CMakeFiles/wedb.dir/main/utils_test.cc.o"
	/usr/bin/g++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT CMakeFiles/wedb.dir/main/utils_test.cc.o -MF CMakeFiles/wedb.dir/main/utils_test.cc.o.d -o CMakeFiles/wedb.dir/main/utils_test.cc.o -c /home/ruanjian/workspace/project_v/wedb/main/utils_test.cc

CMakeFiles/wedb.dir/main/utils_test.cc.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/wedb.dir/main/utils_test.cc.i"
	/usr/bin/g++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/ruanjian/workspace/project_v/wedb/main/utils_test.cc > CMakeFiles/wedb.dir/main/utils_test.cc.i

CMakeFiles/wedb.dir/main/utils_test.cc.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/wedb.dir/main/utils_test.cc.s"
	/usr/bin/g++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/ruanjian/workspace/project_v/wedb/main/utils_test.cc -o CMakeFiles/wedb.dir/main/utils_test.cc.s

CMakeFiles/wedb.dir/cache/hash_table.cc.o: CMakeFiles/wedb.dir/flags.make
CMakeFiles/wedb.dir/cache/hash_table.cc.o: ../cache/hash_table.cc
CMakeFiles/wedb.dir/cache/hash_table.cc.o: CMakeFiles/wedb.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/ruanjian/workspace/project_v/wedb/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Building CXX object CMakeFiles/wedb.dir/cache/hash_table.cc.o"
	/usr/bin/g++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT CMakeFiles/wedb.dir/cache/hash_table.cc.o -MF CMakeFiles/wedb.dir/cache/hash_table.cc.o.d -o CMakeFiles/wedb.dir/cache/hash_table.cc.o -c /home/ruanjian/workspace/project_v/wedb/cache/hash_table.cc

CMakeFiles/wedb.dir/cache/hash_table.cc.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/wedb.dir/cache/hash_table.cc.i"
	/usr/bin/g++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/ruanjian/workspace/project_v/wedb/cache/hash_table.cc > CMakeFiles/wedb.dir/cache/hash_table.cc.i

CMakeFiles/wedb.dir/cache/hash_table.cc.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/wedb.dir/cache/hash_table.cc.s"
	/usr/bin/g++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/ruanjian/workspace/project_v/wedb/cache/hash_table.cc -o CMakeFiles/wedb.dir/cache/hash_table.cc.s

CMakeFiles/wedb.dir/struct/FSet.cc.o: CMakeFiles/wedb.dir/flags.make
CMakeFiles/wedb.dir/struct/FSet.cc.o: ../struct/FSet.cc
CMakeFiles/wedb.dir/struct/FSet.cc.o: CMakeFiles/wedb.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/ruanjian/workspace/project_v/wedb/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_3) "Building CXX object CMakeFiles/wedb.dir/struct/FSet.cc.o"
	/usr/bin/g++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT CMakeFiles/wedb.dir/struct/FSet.cc.o -MF CMakeFiles/wedb.dir/struct/FSet.cc.o.d -o CMakeFiles/wedb.dir/struct/FSet.cc.o -c /home/ruanjian/workspace/project_v/wedb/struct/FSet.cc

CMakeFiles/wedb.dir/struct/FSet.cc.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/wedb.dir/struct/FSet.cc.i"
	/usr/bin/g++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/ruanjian/workspace/project_v/wedb/struct/FSet.cc > CMakeFiles/wedb.dir/struct/FSet.cc.i

CMakeFiles/wedb.dir/struct/FSet.cc.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/wedb.dir/struct/FSet.cc.s"
	/usr/bin/g++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/ruanjian/workspace/project_v/wedb/struct/FSet.cc -o CMakeFiles/wedb.dir/struct/FSet.cc.s

CMakeFiles/wedb.dir/struct/dsn_hash_set.cc.o: CMakeFiles/wedb.dir/flags.make
CMakeFiles/wedb.dir/struct/dsn_hash_set.cc.o: ../struct/dsn_hash_set.cc
CMakeFiles/wedb.dir/struct/dsn_hash_set.cc.o: CMakeFiles/wedb.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/ruanjian/workspace/project_v/wedb/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_4) "Building CXX object CMakeFiles/wedb.dir/struct/dsn_hash_set.cc.o"
	/usr/bin/g++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT CMakeFiles/wedb.dir/struct/dsn_hash_set.cc.o -MF CMakeFiles/wedb.dir/struct/dsn_hash_set.cc.o.d -o CMakeFiles/wedb.dir/struct/dsn_hash_set.cc.o -c /home/ruanjian/workspace/project_v/wedb/struct/dsn_hash_set.cc

CMakeFiles/wedb.dir/struct/dsn_hash_set.cc.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/wedb.dir/struct/dsn_hash_set.cc.i"
	/usr/bin/g++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/ruanjian/workspace/project_v/wedb/struct/dsn_hash_set.cc > CMakeFiles/wedb.dir/struct/dsn_hash_set.cc.i

CMakeFiles/wedb.dir/struct/dsn_hash_set.cc.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/wedb.dir/struct/dsn_hash_set.cc.s"
	/usr/bin/g++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/ruanjian/workspace/project_v/wedb/struct/dsn_hash_set.cc -o CMakeFiles/wedb.dir/struct/dsn_hash_set.cc.s

CMakeFiles/wedb.dir/utils/bloom_filter.cc.o: CMakeFiles/wedb.dir/flags.make
CMakeFiles/wedb.dir/utils/bloom_filter.cc.o: ../utils/bloom_filter.cc
CMakeFiles/wedb.dir/utils/bloom_filter.cc.o: CMakeFiles/wedb.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/ruanjian/workspace/project_v/wedb/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_5) "Building CXX object CMakeFiles/wedb.dir/utils/bloom_filter.cc.o"
	/usr/bin/g++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT CMakeFiles/wedb.dir/utils/bloom_filter.cc.o -MF CMakeFiles/wedb.dir/utils/bloom_filter.cc.o.d -o CMakeFiles/wedb.dir/utils/bloom_filter.cc.o -c /home/ruanjian/workspace/project_v/wedb/utils/bloom_filter.cc

CMakeFiles/wedb.dir/utils/bloom_filter.cc.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/wedb.dir/utils/bloom_filter.cc.i"
	/usr/bin/g++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/ruanjian/workspace/project_v/wedb/utils/bloom_filter.cc > CMakeFiles/wedb.dir/utils/bloom_filter.cc.i

CMakeFiles/wedb.dir/utils/bloom_filter.cc.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/wedb.dir/utils/bloom_filter.cc.s"
	/usr/bin/g++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/ruanjian/workspace/project_v/wedb/utils/bloom_filter.cc -o CMakeFiles/wedb.dir/utils/bloom_filter.cc.s

# Object files for target wedb
wedb_OBJECTS = \
"CMakeFiles/wedb.dir/main/utils_test.cc.o" \
"CMakeFiles/wedb.dir/cache/hash_table.cc.o" \
"CMakeFiles/wedb.dir/struct/FSet.cc.o" \
"CMakeFiles/wedb.dir/struct/dsn_hash_set.cc.o" \
"CMakeFiles/wedb.dir/utils/bloom_filter.cc.o"

# External object files for target wedb
wedb_EXTERNAL_OBJECTS =

wedb: CMakeFiles/wedb.dir/main/utils_test.cc.o
wedb: CMakeFiles/wedb.dir/cache/hash_table.cc.o
wedb: CMakeFiles/wedb.dir/struct/FSet.cc.o
wedb: CMakeFiles/wedb.dir/struct/dsn_hash_set.cc.o
wedb: CMakeFiles/wedb.dir/utils/bloom_filter.cc.o
wedb: CMakeFiles/wedb.dir/build.make
wedb: CMakeFiles/wedb.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/ruanjian/workspace/project_v/wedb/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_6) "Linking CXX executable wedb"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/wedb.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/wedb.dir/build: wedb
.PHONY : CMakeFiles/wedb.dir/build

CMakeFiles/wedb.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/wedb.dir/cmake_clean.cmake
.PHONY : CMakeFiles/wedb.dir/clean

CMakeFiles/wedb.dir/depend:
	cd /home/ruanjian/workspace/project_v/wedb/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/ruanjian/workspace/project_v/wedb /home/ruanjian/workspace/project_v/wedb /home/ruanjian/workspace/project_v/wedb/build /home/ruanjian/workspace/project_v/wedb/build /home/ruanjian/workspace/project_v/wedb/build/CMakeFiles/wedb.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/wedb.dir/depend

