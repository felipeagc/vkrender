set(
  SCENE_SOURCES
  scene/parser.cpp
  scene/lexer.cpp
  scene/driver.cpp
  )

add_library(
  scene
  SHARED
  ${SCENE_SOURCES}
  )

target_include_directories(
  scene
  PUBLIC
  "${CMAKE_CURRENT_SOURCE_DIR}"
  )

target_link_libraries(
  scene

  PRIVATE
  m
  glm
  )

cotire(scene)
