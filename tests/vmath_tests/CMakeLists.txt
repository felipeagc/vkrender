add_executable(
  vmath_tests
  vec3.cpp
  vec4.cpp
  mat4.cpp
)

target_link_libraries(
  vmath_tests
  vmath
  gtest
  gtest_main
)

gtest_discover_tests(vmath_tests)
