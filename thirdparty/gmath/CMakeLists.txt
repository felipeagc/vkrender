add_library(gmath INTERFACE)
target_include_directories(gmath INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(gmath INTERFACE m)
