add_executable(
  ftl_tests
  fixed_vector.cpp
)

target_link_libraries(
  ftl_tests
  ftl
  gtest
  gtest_main
)

gtest_discover_tests(ftl_tests)
