add_library(fstl INTERFACE)

target_include_directories(
  fstl INTERFACE
  "${CMAKE_CURRENT_SOURCE_DIR}"
)

target_link_libraries(
  fstl INTERFACE
  fmt
)
