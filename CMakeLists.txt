cmake_minimum_required(VERSION 3.11)
project(vkrender
	VERSION 0.1
	LANGUAGES C CXX)

if(MSVC)
	add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif()

find_package(Vulkan REQUIRED)

include(FetchContent)

set(GLFW_BUILD_EXAMPLES OFF)
set(GLFW_BUILD_TESTS OFF)
set(GLFW_BUILD_DOCS OFF)

FetchContent_Declare(
	glfw
	URL      https://github.com/glfw/glfw/releases/download/3.2.1/glfw-3.2.1.zip
	URL_HASH MD5=824c99eea073bdd6d2fec76b538f79af)

FetchContent_GetProperties(glfw)
if(NOT glfw_POPULATED)
    FetchContent_Populate(glfw)
    add_subdirectory(${glfw_SOURCE_DIR} ${glfw_BINARY_DIR})
endif()

set(GLFW_LIBRARIES glfw)

add_subdirectory(thirdparty)
add_subdirectory(util)
add_subdirectory(renderer)
add_subdirectory(engine)
add_subdirectory(examples)
