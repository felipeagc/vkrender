set(
  ENGINE_SOURCES
  engine/world.cpp
  engine/pipelines.cpp
  engine/pbr.cpp

  engine/camera.cpp
  engine/environment.cpp
  engine/mesh.cpp

  engine/systems/fps_camera_system.cpp

  engine/asset.cpp
  engine/asset_manager.cpp
  engine/assets/environment_asset.cpp
  engine/assets/mesh_asset.cpp
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
  vkm
  cgltf
  )

target_include_directories(
  engine
  PUBLIC
  "${CMAKE_CURRENT_SOURCE_DIR}"
  )
