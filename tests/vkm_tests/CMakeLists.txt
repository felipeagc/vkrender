add_executable(
  vkm_tests
  vec3.cpp
  vec4.cpp
  mat4.cpp
)

target_link_libraries(
  vkm_tests
  vkm
  gtest
  gtest_main
)

gtest_discover_tests(vkm_tests)
