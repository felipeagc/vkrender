add_library(
  cgltf
  cgltf/cgltf.c
  )

target_include_directories(
  cgltf
  PUBLIC
  "${CMAKE_CURRENT_SOURCE_DIR}"
  )
