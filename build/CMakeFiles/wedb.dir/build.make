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

CMakeFiles/wedb.dir/lsm/bloom_filter.cc.o: CMakeFiles/wedb.dir/flags.make
CMakeFiles/wedb.dir/lsm/bloom_filter.cc.o: ../lsm/bloom_filter.cc
CMakeFiles/wedb.dir/lsm/bloom_filter.cc.o: CMakeFiles/wedb.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/ruanjian/workspace/project_v/wedb/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Building CXX object CMakeFiles/wedb.dir/lsm/bloom_filter.cc.o"
	/usr/bin/g++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT CMakeFiles/wedb.dir/lsm/bloom_filter.cc.o -MF CMakeFiles/wedb.dir/lsm/bloom_filter.cc.o.d -o CMakeFiles/wedb.dir/lsm/bloom_filter.cc.o -c /home/ruanjian/workspace/project_v/wedb/lsm/bloom_filter.cc

CMakeFiles/wedb.dir/lsm/bloom_filter.cc.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/wedb.dir/lsm/bloom_filter.cc.i"
	/usr/bin/g++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/ruanjian/workspace/project_v/wedb/lsm/bloom_filter.cc > CMakeFiles/wedb.dir/lsm/bloom_filter.cc.i

CMakeFiles/wedb.dir/lsm/bloom_filter.cc.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/wedb.dir/lsm/bloom_filter.cc.s"
	/usr/bin/g++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/ruanjian/workspace/project_v/wedb/lsm/bloom_filter.cc -o CMakeFiles/wedb.dir/lsm/bloom_filter.cc.s

CMakeFiles/wedb.dir/lsm/sstable.cc.o: CMakeFiles/wedb.dir/flags.make
CMakeFiles/wedb.dir/lsm/sstable.cc.o: ../lsm/sstable.cc
CMakeFiles/wedb.dir/lsm/sstable.cc.o: CMakeFiles/wedb.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/ruanjian/workspace/project_v/wedb/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_3) "Building CXX object CMakeFiles/wedb.dir/lsm/sstable.cc.o"
	/usr/bin/g++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT CMakeFiles/wedb.dir/lsm/sstable.cc.o -MF CMakeFiles/wedb.dir/lsm/sstable.cc.o.d -o CMakeFiles/wedb.dir/lsm/sstable.cc.o -c /home/ruanjian/workspace/project_v/wedb/lsm/sstable.cc

CMakeFiles/wedb.dir/lsm/sstable.cc.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/wedb.dir/lsm/sstable.cc.i"
	/usr/bin/g++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/ruanjian/workspace/project_v/wedb/lsm/sstable.cc > CMakeFiles/wedb.dir/lsm/sstable.cc.i

CMakeFiles/wedb.dir/lsm/sstable.cc.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/wedb.dir/lsm/sstable.cc.s"
	/usr/bin/g++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/ruanjian/workspace/project_v/wedb/lsm/sstable.cc -o CMakeFiles/wedb.dir/lsm/sstable.cc.s

CMakeFiles/wedb.dir/lsm/memory_db.cc.o: CMakeFiles/wedb.dir/flags.make
CMakeFiles/wedb.dir/lsm/memory_db.cc.o: ../lsm/memory_db.cc
CMakeFiles/wedb.dir/lsm/memory_db.cc.o: CMakeFiles/wedb.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/ruanjian/workspace/project_v/wedb/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_4) "Building CXX object CMakeFiles/wedb.dir/lsm/memory_db.cc.o"
	/usr/bin/g++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT CMakeFiles/wedb.dir/lsm/memory_db.cc.o -MF CMakeFiles/wedb.dir/lsm/memory_db.cc.o.d -o CMakeFiles/wedb.dir/lsm/memory_db.cc.o -c /home/ruanjian/workspace/project_v/wedb/lsm/memory_db.cc

CMakeFiles/wedb.dir/lsm/memory_db.cc.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/wedb.dir/lsm/memory_db.cc.i"
	/usr/bin/g++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/ruanjian/workspace/project_v/wedb/lsm/memory_db.cc > CMakeFiles/wedb.dir/lsm/memory_db.cc.i

CMakeFiles/wedb.dir/lsm/memory_db.cc.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/wedb.dir/lsm/memory_db.cc.s"
	/usr/bin/g++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/ruanjian/workspace/project_v/wedb/lsm/memory_db.cc -o CMakeFiles/wedb.dir/lsm/memory_db.cc.s

# Object files for target wedb
wedb_OBJECTS = \
"CMakeFiles/wedb.dir/main/utils_test.cc.o" \
"CMakeFiles/wedb.dir/lsm/bloom_filter.cc.o" \
"CMakeFiles/wedb.dir/lsm/sstable.cc.o" \
"CMakeFiles/wedb.dir/lsm/memory_db.cc.o"

# External object files for target wedb
wedb_EXTERNAL_OBJECTS =

wedb: CMakeFiles/wedb.dir/main/utils_test.cc.o
wedb: CMakeFiles/wedb.dir/lsm/bloom_filter.cc.o
wedb: CMakeFiles/wedb.dir/lsm/sstable.cc.o
wedb: CMakeFiles/wedb.dir/lsm/memory_db.cc.o
wedb: CMakeFiles/wedb.dir/build.make
wedb: CMakeFiles/wedb.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/ruanjian/workspace/project_v/wedb/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_5) "Linking CXX executable wedb"
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

