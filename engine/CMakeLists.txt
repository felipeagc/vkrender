add_library(
	engine

	engine/engine.c
	engine/engine.h
	engine/util.h
	engine/world.c
	engine/world.h
	engine/asset_manager.c
	engine/asset_manager.h
	engine/filesystem.c
	engine/filesystem.h

	engine/inspector.c
	engine/inspector.h

	engine/pipelines.c
	engine/pipelines.h
	engine/pbr.c
	engine/pbr.h

	engine/camera.c
	engine/camera.h
	engine/environment.c
	engine/environment.h

	engine/components/component_types.c
	engine/components/component_types.h
	engine/components/mesh_component.c
	engine/components/mesh_component.h
	engine/components/transform_component.c
	engine/components/transform_component.h
	engine/components/gltf_model_component.c
	engine/components/gltf_model_component.h

	engine/systems/fps_camera_system.c
	engine/systems/fps_camera_system.h
	engine/systems/picking_system.c
	engine/systems/picking_system.h

	engine/assets/env_file.h
	engine/assets/asset_types.c
	engine/assets/asset_types.h
	engine/assets/environment_asset.c
	engine/assets/environment_asset.h
	engine/assets/mesh_asset.c
	engine/assets/mesh_asset.h
	engine/assets/gltf_model_asset.c
	engine/assets/gltf_model_asset.h
	engine/assets/pbr_material_asset.c
	engine/assets/pbr_material_asset.h)

target_link_libraries(
	engine
	PUBLIC
	renderer
	cgltf
	physfs-static)

target_include_directories(
	engine
	PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR})
