find_package(GTest)

if(GTEST_FOUND)
  add_subdirectory(container_tests)
endif(GTEST_FOUND)
