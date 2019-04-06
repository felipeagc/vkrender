add_library(vma vk_mem_alloc.cpp)

target_link_libraries(
	vma PUBLIC ${Vulkan_LIBRARIES})

target_include_directories(
	vma
	PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR}
	${Vulkan_INCLUDE_DIRS})
