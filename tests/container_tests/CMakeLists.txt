add_executable(
  container_tests
  fixed_vector.cpp
)

target_link_libraries(
  container_tests
  vkrender
  fstl
  gtest
  gtest_main
)

gtest_discover_tests(container_tests)
