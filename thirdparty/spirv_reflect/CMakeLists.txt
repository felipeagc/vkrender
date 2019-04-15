add_library(
	spirv_reflect
	spirv_reflect.c
	spirv_reflect.h
	spirv.h)
target_link_libraries(spirv_reflect PUBLIC ${Vulkan_LIBRARIES})
target_include_directories(spirv_reflect PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
