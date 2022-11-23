project(wedb)

target_include_directories(wedb PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/struct)

target_sources(wedb PRIVATE
		${CMAKE_CURRENT_SOURCE_DIR}/struct/./FSet.cc
		${CMAKE_CURRENT_SOURCE_DIR}/struct/./dsn_hash_set.cc)
