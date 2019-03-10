tinycthread_deps = [dependency('threads')]

tinycthread_lib = library(
  'tinycthread',
  ['tinycthread.c'],
  dependencies: tinycthread_deps)


tinycthread_dep = declare_dependency(
  include_directories: include_directories('.'),
  dependencies: tinycthread_deps,
  link_with: tinycthread_lib)
