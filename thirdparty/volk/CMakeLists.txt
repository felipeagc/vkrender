add_library(
	volk
	volk.c
	volk.h)

target_include_directories(
	volk
	PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR}
	${Vulkan_INCLUDE_DIRS})
