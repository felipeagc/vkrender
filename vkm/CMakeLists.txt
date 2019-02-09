add_library(vkm INTERFACE)

target_include_directories(
  vkm INTERFACE
  "${CMAKE_CURRENT_SOURCE_DIR}"
)

target_link_libraries(
  vkm INTERFACE m
)
