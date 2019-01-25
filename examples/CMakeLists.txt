macro(add_example name)
  add_executable(${name} ${name}.cpp)
  target_link_libraries(${name} PRIVATE engine)
  cotire(${name})
endmacro()

add_example(gltf_ecs)
add_example(mandelbrot)
add_example(rewrite)
