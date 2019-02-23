macro(add_example name)
  add_executable(${name} ${name}.cpp)

  # TODO: not working:
  # target_compile_options(${name} PRIVATE -fsanitize=memory -fno-omit-frame-pointer)
  # target_link_options(${name} PRIVATE -fsanitize=memory -fno-omit-frame-pointer)

  # target_compile_options(${name} PRIVATE -fsanitize=address -fno-omit-frame-pointer)
  # target_link_options(${name} PRIVATE -fsanitize=address -fno-omit-frame-pointer)

  target_link_libraries(${name} PRIVATE engine)
endmacro()

add_example(rewrite)
add_example(vkm_example)
