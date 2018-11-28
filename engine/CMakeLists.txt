set(
  ENGINE_SOURCES
  engine/gltf_model.cpp
  engine/camera.cpp
  engine/light_manager.cpp
  engine/billboard.cpp
  engine/asset_manager.cpp
  engine/gltf_model_instance.cpp
  engine/imgui_utils.cpp
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
