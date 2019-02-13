add_library(
  vma
  vk_mem_alloc.cpp
  )

target_link_libraries(
  vma
  ${VULKAN_LIBRARIES}
  )

target_include_directories(
  vma
  PUBLIC
  "${CMAKE_CURRENT_SOURCE_DIR}"
  "${VULKAN_INCLUDE_DIRS}"
  )
