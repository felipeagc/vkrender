set(
  UTIL_SOURCES
  util/bump_allocator.cpp
  )

add_library(
  util
  SHARED
  ${UTIL_SOURCES}
  )

target_link_libraries(
  util
  )

target_include_directories(
  util
  PUBLIC
  "${CMAKE_CURRENT_SOURCE_DIR}"
  )
