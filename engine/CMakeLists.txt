set(
  ENGINE_SOURCES
  engine/world.cpp
  engine/camera.cpp
  engine/systems/fps_camera_system.cpp
  engine/pipelines.cpp
  engine/environment.cpp

  engine/asset.cpp
  engine/assets/environment_asset.cpp
  )

add_library(
  engine
  SHARED
  ${ENGINE_SOURCES}
  )

target_link_libraries(
  engine

  PUBLIC
  util
  renderer
  vmath
  )

target_include_directories(
  engine
  PUBLIC
  "${CMAKE_CURRENT_SOURCE_DIR}"
  )
