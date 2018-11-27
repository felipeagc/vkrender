macro(add_example name)
  add_executable(${name}_example ${name}.cpp)
  target_link_libraries(${name}_example PRIVATE vkrender fstl ecs)
endmacro()

add_example(triangle)
add_example(gltf)
add_example(gltf_lit)
add_example(ecs)
