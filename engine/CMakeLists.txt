set(
  ENGINE_SOURCES
  engine/world.cpp
  engine/asset_manager.cpp
  engine/inspector.cpp
  engine/pipelines.cpp
  engine/pbr.cpp

  engine/camera.cpp
  engine/environment.cpp

  engine/components/component_types.cpp
  engine/components/mesh_component.cpp
  engine/components/transform_component.cpp

  engine/systems/fps_camera_system.cpp

  engine/assets/asset_types.cpp
  engine/assets/environment_asset.cpp
  engine/assets/mesh_asset.cpp
  engine/assets/pbr_material_asset.cpp
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
