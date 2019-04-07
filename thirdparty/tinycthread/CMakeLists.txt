find_package(Threads REQUIRED)

add_library(tinycthread tinycthread.c)
target_link_libraries(tinycthread PUBLIC Threads::Threads)
target_include_directories(tinycthread PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
