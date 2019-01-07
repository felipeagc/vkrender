add_executable(
  sdf_tests
  scene.cpp
)

target_link_libraries(
  sdf_tests
  sdf
  gtest
  gtest_main
)

gtest_discover_tests(sdf_tests)
