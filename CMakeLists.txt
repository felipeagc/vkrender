cmake_minimum_required(VERSION 3.11)
project(vkrender
	VERSION 0.1
	LANGUAGES C CXX)

if(MSVC)
	add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif()

find_package(Vulkan REQUIRED)

add_subdirectory(thirdparty)
add_subdirectory(util)
add_subdirectory(renderer)
add_subdirectory(engine)
add_subdirectory(examples)
