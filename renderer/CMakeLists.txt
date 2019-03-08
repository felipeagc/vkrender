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
  renderer/shader.cpp
  renderer/pipeline.cpp
  renderer/canvas.cpp
  renderer/imgui.cpp
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
  fstd
  util
  gmath
  vma
  tinycthread

  PRIVATE
  stb_image
  )
