project(wedb)

target_include_directories(wedb PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/lsm)

target_sources(wedb PRIVATE
		${CMAKE_CURRENT_SOURCE_DIR}/lsm/./bloom_filter.cc
		${CMAKE_CURRENT_SOURCE_DIR}/lsm/./sstable.cc
		${CMAKE_CURRENT_SOURCE_DIR}/lsm/./memory_db.cc)
