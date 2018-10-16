add_executable(
  container_tests
  smallvec.cpp
)

target_link_libraries(
  container_tests
  vkrender
  GTest::GTest
  GTest::Main
)

gtest_discover_tests(container_tests)
