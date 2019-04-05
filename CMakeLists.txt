cmake_minimum_required(VERSION 3.10)
project(vkrender
	VERSION 0.1
	LANGUAGES C CXX)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")
set(BUILD_SHARED_LIBS ON)

find_package(SDL2 REQUIRED)
find_package(Vulkan REQUIRED)

add_subdirectory(thirdparty)
add_subdirectory(util)
add_subdirectory(renderer)
add_subdirectory(engine)
add_subdirectory(examples)
