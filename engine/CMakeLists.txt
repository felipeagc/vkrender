set(
  ENGINE_SOURCES
  engine/gltf_model.cpp
  engine/light_manager.cpp
  engine/asset_manager.cpp
  engine/imgui_utils.cpp

  engine/camera_component.cpp
  engine/gltf_model_component.cpp
  engine/billboard_component.cpp
  )

add_library(
  engine
  SHARED
  ${ENGINE_SOURCES}
  )

target_link_libraries(
  engine

  PUBLIC
  fstl
  renderer
  ecs

  PRIVATE
  tinygltf
  rapidjson
  )

target_include_directories(
  engine
  PUBLIC
  "${CMAKE_CURRENT_SOURCE_DIR}"
  )
