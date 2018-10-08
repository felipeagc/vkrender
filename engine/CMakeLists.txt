# Dependencies
find_package(SDL2 REQUIRED)
find_package(Vulkan REQUIRED)
find_package(GLM REQUIRED)

set(
  VKRENDER_SOURCES
  vkr/context.cpp
  vkr/window.cpp
  vkr/pipeline.cpp
  vkr/buffer.cpp
  vkr/commandbuffer.cpp
  vkr/texture.cpp
  )

add_library(vkrender ${VKRENDER_SOURCES})

target_include_directories(
  vkrender
  PUBLIC
  "${CMAKE_CURRENT_SOURCE_DIR}"
  ${SDL2_INCLUDE_DIR}
  ${Vulkan_INCLUDE_DIR}
  ${GLM_INCLUDE_DIRS}
  )

target_link_libraries(
  vkrender
  PUBLIC
  vma
  stb_image
  spirv-cross-core
  spirv-cross-glsl
  spirv-cross-cpp
  spirv-cross-reflect
  fmt
  ${SDL2_LIBRARY}
  ${Vulkan_LIBRARIES}
  )
