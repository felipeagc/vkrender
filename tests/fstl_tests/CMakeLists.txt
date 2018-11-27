add_executable(
  fstl_tests
  fixed_vector.cpp
)

target_link_libraries(
  fstl_tests
  fstl
  gtest
  gtest_main
)

gtest_discover_tests(fstl_tests)
