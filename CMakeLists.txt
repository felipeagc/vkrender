cmake_minimum_required(VERSION 3.7)
project(vkrender)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")

add_subdirectory(thirdparty)
add_subdirectory(engine)
add_subdirectory(game)
