set(
  ENGINE_SOURCES
  engine/asset_manager.cpp
  engine/imgui_utils.cpp
  engine/file_watcher.cpp
  engine/scene.cpp
  engine/pipelines.cpp

  engine/components/camera_component.cpp
  engine/components/gltf_model_component.cpp
  engine/components/billboard_component.cpp
  engine/components/environment_component.cpp
  engine/components/transform_component.cpp
  engine/components/light_component.cpp

  engine/systems/gltf_model_system.cpp
  engine/systems/fps_camera_system.cpp
  engine/systems/billboard_system.cpp
  engine/systems/skybox_system.cpp
  engine/systems/lighting_system.cpp
  engine/systems/entity_inspector_system.cpp

  engine/assets/gltf_model_asset.cpp
  engine/assets/texture_asset.cpp
  engine/assets/environment_asset.cpp
  engine/assets/shape_asset.cpp
  )

add_library(
  engine
  SHARED
  ${ENGINE_SOURCES}
  )

target_link_libraries(
  engine

  PUBLIC
  ftl
  renderer
  ecs
  sdf

  PRIVATE
  tinygltf
  )

target_include_directories(
  engine
  PUBLIC
  "${CMAKE_CURRENT_SOURCE_DIR}"
  )

cotire(engine)
