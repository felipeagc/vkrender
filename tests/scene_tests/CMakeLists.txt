add_executable(
  scene_tests
  scene.cpp
)

target_link_libraries(
  scene_tests
  scene
  gtest
  gtest_main
)

gtest_discover_tests(scene_tests)
