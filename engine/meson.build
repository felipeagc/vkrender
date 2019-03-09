engine_sources = [
  'engine/engine.c',
  'engine/world.c',
  'engine/asset_manager.c',
  'engine/inspector.c',
  'engine/pipelines.c',
  'engine/pbr.c',

  'engine/camera.c',
  'engine/environment.c',

  'engine/components/component_types.c',
  'engine/components/mesh_component.c',
  'engine/components/transform_component.c',

  'engine/systems/fps_camera_system.c',

  'engine/assets/asset_types.c',
  'engine/assets/environment_asset.c',
  'engine/assets/mesh_asset.c',
  'engine/assets/pbr_material_asset.c'
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
