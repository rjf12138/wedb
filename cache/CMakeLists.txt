project(wedb)

target_include_directories(wedb PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/cache)

target_sources(wedb PRIVATE
		${CMAKE_CURRENT_SOURCE_DIR}/cache/./hash_table.cc)
