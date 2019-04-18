add_library(
	fstd
	fstd.c
	fstd_alloc.h
	fstd_map.h
	fstd_bitset.h
	fstd_util.h)

target_include_directories(
	fstd
	PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR})
