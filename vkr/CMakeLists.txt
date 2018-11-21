# Dependencies
find_package(SDL2 REQUIRED)
find_package(Vulkan REQUIRED)

set(
  VKRENDER_SOURCES
  vkr/context.cpp
  vkr/window.cpp
  vkr/buffer.cpp
  vkr/texture.cpp
  vkr/shader_compilation.cpp
  vkr/gltf_model.cpp
  vkr/camera.cpp
  vkr/descriptor_manager.cpp
  vkr/vertex_format.cpp
  vkr/shader.cpp
  vkr/imgui_utils.cpp
  vkr/lighting.cpp
  vkr/pipeline.cpp
  )

add_library(
  vkrender
  SHARED
  ${VKRENDER_SOURCES}
  )

target_include_directories(
  vkrender
  PUBLIC
  "${CMAKE_CURRENT_SOURCE_DIR}"
  ${SDL2_INCLUDE_DIR}
  ${Vulkan_INCLUDE_DIR}
  )

target_link_libraries(
  vkrender

  PUBLIC
  imgui
  ${SDL2_LIBRARY}
  ${Vulkan_LIBRARIES}
  fstl
  vma

  PRIVATE
  stb_image
  spirv-cross-core
  spirv-cross-glsl
  spirv-cross-cpp
  spirv-cross-reflect
  glslang
  glslang-default-resource-limits
  SPIRV
  tinygltf
  )

cotire(vkrender)
