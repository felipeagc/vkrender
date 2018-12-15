# Dependencies
find_package(SDL2 REQUIRED)
find_package(Vulkan REQUIRED)

set(
  RENDERER_SOURCES
  renderer/context.cpp
  renderer/window.cpp
  renderer/util.cpp
  renderer/buffer.cpp
  renderer/texture.cpp
  renderer/cubemap.cpp
  renderer/shader_compilation.cpp
  renderer/descriptor_manager.cpp
  renderer/vertex_format.cpp
  renderer/shader.cpp
  renderer/pipeline.cpp
  )

add_library(
  renderer
  SHARED
  ${RENDERER_SOURCES}
  )

target_include_directories(
  renderer
  PUBLIC
  "${CMAKE_CURRENT_SOURCE_DIR}"
  ${SDL2_INCLUDE_DIR}
  ${Vulkan_INCLUDE_DIR}
  )

target_link_libraries(
  renderer

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
  )

cotire(renderer)
