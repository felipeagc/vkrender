add_library(
  fstd
  fstd.c
  )

target_include_directories(
  fstd
  PUBLIC
  "${CMAKE_CURRENT_SOURCE_DIR}"
  )
