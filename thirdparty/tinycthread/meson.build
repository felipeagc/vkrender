tinycthread_lib = library('tinycthread',
  ['tinycthread.c'],
  dependencies: [dependency('threads')])

tinycthread_dep = declare_dependency(
  include_directories: include_directories('.'),
  link_with: tinycthread_lib)
