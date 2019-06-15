add_library(
	engine

	engine/all.h

	engine/util.h

	engine/engine.c
	engine/engine.h
	engine/scene.c
	engine/scene.h
	engine/entity_manager.c
	engine/entity_manager.h
	engine/asset_manager.c
	engine/asset_manager.h
	engine/filesystem.c
	engine/filesystem.h

	engine/imgui.c
	engine/imgui.h

	engine/picker.c
	engine/picker.h
	engine/inspector.c
	engine/inspector.h

	engine/pipelines.c
	engine/pipelines.h

	engine/camera.c
	engine/camera.h
	engine/environment.c
	engine/environment.h

	engine/task_scheduler.c
	engine/task_scheduler.h

	engine/serializer.c
	engine/serializer.h

	engine/util/tinyktx.c
	engine/util/tinyktx.h

	engine/comps/comp_types.c
	engine/comps/comp_types.h
	engine/comps/transform_comp.c
	engine/comps/transform_comp.h
	engine/comps/point_light_comp.c
	engine/comps/point_light_comp.h
	engine/comps/renderable_comp.c
	engine/comps/renderable_comp.h
	engine/comps/mesh_comp.c
	engine/comps/mesh_comp.h
	engine/comps/gltf_comp.c
	engine/comps/gltf_comp.h
	engine/comps/terrain_comp.c
	engine/comps/terrain_comp.h

	engine/systems/rendering_system.c
	engine/systems/rendering_system.h
	engine/systems/fps_camera_system.c
	engine/systems/fps_camera_system.h
	engine/systems/light_system.c
	engine/systems/light_system.h

	engine/assets/asset_types.c
	engine/assets/asset_types.h
	engine/assets/pipeline_asset.c
	engine/assets/pipeline_asset.h
	engine/assets/image_asset.c
	engine/assets/image_asset.h
	engine/assets/mesh_asset.c
	engine/assets/mesh_asset.h
	engine/assets/gltf_asset.c
	engine/assets/gltf_asset.h
	engine/assets/pbr_material_asset.c
	engine/assets/pbr_material_asset.h
	)

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

if(NOT MSVC)
	target_compile_options(engine PRIVATE
		-Wall
		-Wextra
	  -Wno-missing-braces
	  -Wno-unused-parameter
		-mavx
		-maes
		)
endif()
