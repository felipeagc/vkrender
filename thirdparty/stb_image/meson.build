cc = meson.get_compiler('c')
m_dep = cc.find_library('m', required: false)

stb_image_lib = library('stb_image',
  ['stb_image.c'],
  dependencies: [m_dep])

stb_image_dep = declare_dependency(
  include_directories: include_directories('.'),
  link_with: stb_image_lib)
