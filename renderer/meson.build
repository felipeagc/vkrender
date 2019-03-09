renderer_sources = [
  'renderer/context.c',
  'renderer/window.c',
  'renderer/util.c',
  'renderer/buffer.c',
  'renderer/texture.c',
  'renderer/cubemap.c',
  'renderer/shader.c',
  'renderer/pipeline.c',
  'renderer/canvas.c',
  'renderer/imgui.c',
  'renderer/task_scheduler.c'
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
