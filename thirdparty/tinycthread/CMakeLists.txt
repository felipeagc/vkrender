# Dependencies

add_library(
  tinycthread
  STATIC
  tinycthread.c
  )

target_link_libraries(tinycthread)

target_include_directories(
  tinycthread
  PUBLIC
  "${CMAKE_CURRENT_SOURCE_DIR}"
  )
