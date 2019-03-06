add_library(spirv-reflect spirv_reflect/spirv_reflect.c)

target_link_libraries(spirv-reflect ${VULKAN_LIBRARIES})

target_include_directories(
  spirv-reflect
  PUBLIC
  "${CMAKE_CURRENT_SOURCE_DIR}"
  )
