add_library(
	imgui
	imgui.cpp
	cimgui.cpp
	imgui_demo.cpp
	imgui_draw.cpp
	imgui_impl_sdl.cpp
	imgui_impl_vulkan.cpp
	imgui_widgets.cpp)

target_link_libraries(
	imgui
	PUBLIC
	${Vulkan_LIBRARIES} 
	${SDL2_LIBRARY})

target_include_directories(
	imgui
	PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR}
	${Vulkan_INCLUDE_DIRS})
