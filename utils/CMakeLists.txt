project(wedb)

target_include_directories(wedb PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/utils)

target_sources(wedb PRIVATE
		${CMAKE_CURRENT_SOURCE_DIR}/utils/./bloom_filter.cc)
