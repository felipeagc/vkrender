add_executable(
  container_tests
  smallvec.cpp
)

target_link_libraries(
  container_tests
  vkrender
  gtest
  gtest_main
)

gtest_discover_tests(container_tests)
