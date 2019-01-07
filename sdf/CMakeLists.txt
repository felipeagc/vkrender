set(
  SCENE_SOURCES
  sdf/parser.cpp
  )

add_library(
  sdf 
  SHARED
  ${SCENE_SOURCES}
  )

target_include_directories(
  sdf
  PUBLIC
  "${CMAKE_CURRENT_SOURCE_DIR}"
  )

target_link_libraries(
  sdf

  PRIVATE
  m
  glm
  )

cotire(sdf)
