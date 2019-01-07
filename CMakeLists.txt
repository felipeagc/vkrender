cmake_minimum_required(VERSION 3.10)
project(vkrender)

# Enable building libraries as DLLs
set(BUILD_SHARED_LIBS ON)

# Enable -fPIC
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Disable submodule tests
configure_file(${CMAKE_SOURCE_DIR}/CTestCustom.cmake ${CMAKE_BINARY_DIR})

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -pedantic -Wextra -std=c++17")

# Ccache
find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
  set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE "${CCACHE_PROGRAM}")
endif()

# Colored error messages
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
  add_compile_options (-fdiagnostics-color=always)
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
  add_compile_options (-fcolor-diagnostics)
endif()

include(cotire)

add_subdirectory(thirdparty)
add_subdirectory(ftl)
add_subdirectory(ecs)
add_subdirectory(renderer)
add_subdirectory(sdf)
add_subdirectory(engine)

option(VKR_BUILD_EXAMPLES "Build the examples" ON)
if(VKR_BUILD_EXAMPLES)
  add_subdirectory(examples)
endif()

option(VKR_BUILD_TESTS "Build the tests" ON)
if(VKR_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()
