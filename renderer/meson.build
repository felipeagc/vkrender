renderer_sources = [
  'renderer/context.cpp',
  'renderer/window.cpp',
  'renderer/util.cpp',
  'renderer/buffer.cpp',
  'renderer/texture.cpp',
  'renderer/cubemap.cpp',
  'renderer/shader.cpp',
  'renderer/pipeline.cpp',
  'renderer/canvas.cpp',
  'renderer/imgui.cpp',
  'renderer/task_scheduler.cpp'
]

renderer_inc = include_directories('.')

renderer_deps = [
  dependency('sdl2'),
  dependency('vulkan'),
  imgui_dep,
  fstd_dep,
  util_dep,
  gmath_dep,
  vma_dep,
  tinycthread_dep,
  stb_image_dep
]

renderer_lib = library(
  'renderer',
  renderer_sources,
  include_directories: renderer_inc,
  dependencies: renderer_deps)

renderer_dep = declare_dependency(
  include_directories: renderer_inc,
  dependencies: renderer_deps,
  link_with: renderer_lib)
