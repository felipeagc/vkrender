add_library(
	engine
	engine/engine.c
	engine/world.c
	engine/asset_manager.c
	engine/inspector.c
	engine/pipelines.c
	engine/pbr.c

	engine/camera.c
	engine/environment.c

	engine/components/component_types.c
	engine/components/mesh_component.c
	engine/components/transform_component.c
	engine/components/gltf_model_component.c

	engine/systems/fps_camera_system.c

	engine/assets/asset_types.c
	engine/assets/environment_asset.c
	engine/assets/mesh_asset.c
	engine/assets/gltf_model_asset.c
	engine/assets/pbr_material_asset.c)

target_link_libraries(
	engine
	PUBLIC
	renderer
	cgltf)

target_include_directories(engine PUBLIC .)
