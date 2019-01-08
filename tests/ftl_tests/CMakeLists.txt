function(f_add_test test)
  add_executable(${test} ${test}.cpp)
  target_link_libraries(
    ${test}
    ftl
    gtest
    gtest_main
  )
  gtest_discover_tests(${test})
endfunction()

f_add_test(fixed_vector)
f_add_test(string)
f_add_test(string_builder)
f_add_test(stack_allocator)
