add_library(
  tinygltf
  tinygltf.cpp
  )

target_link_libraries(
  tinygltf
  PUBLIC
  stb_image
  )

target_include_directories(
  tinygltf
  PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  )
