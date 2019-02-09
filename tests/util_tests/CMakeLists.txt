function(f_add_test test)
  add_executable(${test} ${test}.cpp)
  target_link_libraries(
    ${test}
    util
    gtest
    gtest_main
  )
  gtest_discover_tests(${test})
endfunction()

f_add_test(bump_allocator)
f_add_test(general_allocator)
