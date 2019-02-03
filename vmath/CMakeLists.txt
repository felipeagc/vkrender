add_library(vmath INTERFACE)

target_include_directories(
  vmath INTERFACE
  "${CMAKE_CURRENT_SOURCE_DIR}"
)

target_link_libraries(
  vmath INTERFACE m
)
