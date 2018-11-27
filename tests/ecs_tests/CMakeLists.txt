add_executable(
  ecs_tests
  ecs.cpp
)

target_link_libraries(
  ecs_tests
  ecs
  gtest
  gtest_main
)

gtest_discover_tests(ecs_tests)
