engine_sources = [
  'engine/engine.cpp',
  'engine/world.cpp',
  'engine/asset_manager.cpp',
  'engine/inspector.cpp',
  'engine/pipelines.cpp',
  'engine/pbr.cpp',

  'engine/camera.cpp',
  'engine/environment.cpp',

  'engine/components/component_types.cpp',
  'engine/components/mesh_component.cpp',
  'engine/components/transform_component.cpp',

  'engine/systems/fps_camera_system.cpp',

  'engine/assets/asset_types.cpp',
  'engine/assets/environment_asset.cpp',
  'engine/assets/mesh_asset.cpp',
  'engine/assets/pbr_material_asset.cpp'
]

engine_inc = include_directories('.')

engine_deps = [
  renderer_dep,
  cgltf_dep,
  gmath_dep,
]

engine_lib = library(
  'engine',
  engine_sources,
  include_directories: engine_inc,
  dependencies: engine_deps)

engine_dep = declare_dependency(
  include_directories: engine_inc,
  dependencies: engine_deps,
  link_with: engine_lib)
