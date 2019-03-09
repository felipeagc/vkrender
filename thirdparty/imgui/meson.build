imgui_lib = library('imgui',
  [
    'imgui.cpp',
    'imgui_demo.cpp',
    'imgui_draw.cpp',
    'imgui_impl_sdl.cpp',
    'imgui_impl_vulkan.cpp',
    'imgui_widgets.cpp'
  ], 
  include_directories: include_directories('.'),
  dependencies: [
    dependency('vulkan'),
    dependency('sdl2')
  ])

imgui_dep = declare_dependency(
  include_directories: include_directories('.'),
  link_with: imgui_lib)
