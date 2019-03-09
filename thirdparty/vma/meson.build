vma_lib = library('vma',
  [ 'vk_mem_alloc.cpp' ], 
  include_directories: include_directories('.'),
  dependencies: [
    dependency('vulkan')
  ])

vma_dep = declare_dependency(
  include_directories: include_directories('.'),
  link_with: vma_lib)
