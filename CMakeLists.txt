cmake_minimum_required(VERSION 3.11)
project(vkrender
	VERSION 0.1
	LANGUAGES C CXX)

if(MSVC)
	add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif()

find_package(Vulkan REQUIRED)

include(FetchContent)

FetchContent_Declare(
	SDL2
	URL      https://libsdl.org/release/SDL2-2.0.9.tar.gz
	URL_HASH MD5=f2ecfba915c54f7200f504d8b48a5dfe)

FetchContent_GetProperties(SDL2)
if(NOT sdl2_POPULATED)
    FetchContent_Populate(SDL2)
    add_subdirectory(${sdl2_SOURCE_DIR} ${sdl2_BINARY_DIR})
endif()

set(SDL2_LIBRARIES SDL2-static SDL2main)

add_subdirectory(thirdparty)
add_subdirectory(util)
add_subdirectory(renderer)
add_subdirectory(engine)
add_subdirectory(examples)
