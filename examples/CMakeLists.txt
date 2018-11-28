macro(add_example name)
  add_executable(${name} ${name}.cpp)
  target_link_libraries(${name} PRIVATE engine)
endmacro()

add_example(triangle)
add_example(gltf_ecs)
