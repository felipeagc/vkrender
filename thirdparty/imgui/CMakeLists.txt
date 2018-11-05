# Dependencies
find_package(SDL2 REQUIRED)
find_package(Vulkan REQUIRED)

set(
  IMGUI_SOURCES
  imgui/imgui.cpp
  imgui/imgui_demo.cpp
  imgui/imgui_draw.cpp
  imgui/imgui_impl_sdl.cpp
  imgui/imgui_impl_vulkan.cpp
  imgui/imgui_widgets.cpp
  )

add_library(imgui ${IMGUI_SOURCES})

target_link_libraries(
  imgui
  PUBLIC
  ${SDL2_LIBRARY}
  ${Vulkan_LIBRARIES}
  )

target_include_directories(
  imgui
  PUBLIC
  "${CMAKE_CURRENT_SOURCE_DIR}"
  ${SDL2_INCLUDE_DIR}
  ${Vulkan_INCLUDE_DIR}
  )
