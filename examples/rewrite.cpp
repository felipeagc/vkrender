#include <engine/asset_manager.hpp>
#include <engine/assets/environment_asset.hpp>
#include <engine/assets/mesh_asset.hpp>
#include <engine/camera.hpp>
#include <engine/components/mesh_component.hpp>
#include <engine/components/transform_component.hpp>
#include <engine/inspector.hpp>
#include <engine/pbr.hpp>
#include <engine/pipelines.hpp>
#include <engine/systems/fps_camera_system.hpp>
#include <engine/world.hpp>
#include <imgui/imgui.h>
#include <renderer/renderer.hpp>
#include <util/array.h>
#include <util/log.h>

int main() {
  re_window_t window;
  re_window_init(&window, "Re-write", 1600, 900);
  re_shader_init_compiler();

  re_imgui_t imgui;
  re_imgui_init(&imgui, &window);

  window.clear_color = {1.0, 1.0, 1.0, 1.0};

  re_pipeline_t pbr_pipeline;
  eg_init_pipeline(
      &pbr_pipeline,
      window.render_target,
      "../shaders/pbr.vert",
      "../shaders/pbr.frag",
      eg_standard_pipeline_parameters());

  re_pipeline_t heightmap_pipeline;
  eg_init_pipeline(
      &heightmap_pipeline,
      window.render_target,
      "../shaders/heightmap.vert",
      "../shaders/heightmap.frag",
      eg_heightmap_pipeline_parameters());

  re_pipeline_t skybox_pipeline;
  eg_init_pipeline(
      &skybox_pipeline,
      window.render_target,
      "../shaders/skybox.vert",
      "../shaders/skybox.frag",
      eg_skybox_pipeline_parameters());

  eg_asset_manager_t asset_manager;
  eg_asset_manager_init(&asset_manager);

  const char *radiance_paths[] = {
      "../assets/ice_lake/radiance_0_1600x800.hdr",
      "../assets/ice_lake/radiance_1_800x400.hdr",
      "../assets/ice_lake/radiance_2_400x200.hdr",
      "../assets/ice_lake/radiance_3_200x100.hdr",
      "../assets/ice_lake/radiance_4_100x50.hdr",
      "../assets/ice_lake/radiance_5_50x25.hdr",
      "../assets/ice_lake/radiance_6_25x12.hdr",
      "../assets/ice_lake/radiance_7_12x6.hdr",
      "../assets/ice_lake/radiance_8_6x3.hdr",
  };

  eg_environment_asset_t *environment_asset =
      eg_asset_alloc(&asset_manager, eg_environment_asset_t);
  eg_environment_asset_init(
      environment_asset,
      1024,
      1024,
      "../assets/ice_lake/skybox.hdr",
      "../assets/ice_lake/irradiance.hdr",
      ARRAYSIZE(radiance_paths),
      radiance_paths,
      "../assets/brdf_lut.png");

  eg_world_t world;
  eg_world_init(&world, environment_asset);

  eg_fps_camera_system_t fps_system;
  eg_fps_camera_system_init(&fps_system);

  re_vertex_t vertices[] = {
      {{-1, -1, 0}, {0, 0, 1}, {0, 0}},
      {{1, -1, 0}, {0, 0, 1}, {1, 0}},
      {{1, 1, 0}, {0, 0, 1}, {1, 1}},
      {{-1, 1, 0}, {0, 0, 1}, {0, 1}},
  };

  uint32_t indices[] = {0, 1, 2, 2, 3, 0};

  eg_mesh_asset_t *mesh_asset = eg_asset_alloc(&asset_manager, eg_mesh_asset_t);
  eg_mesh_asset_init(
      mesh_asset, vertices, ARRAYSIZE(vertices), indices, ARRAYSIZE(indices));

  eg_pbr_material_asset_t *material =
      eg_asset_alloc(&asset_manager, eg_pbr_material_asset_t);
  eg_pbr_material_asset_init(material, NULL, NULL, NULL, NULL, NULL);

  {
    eg_entity_t ent = eg_world_add_entity(&world);
    eg_transform_component_t *transform_comp =
        (eg_transform_component_t *)eg_world_add_comp(
            &world, ent, EG_TRANSFORM_COMPONENT_TYPE);
    eg_transform_component_init(transform_comp);
    eg_mesh_component_t *mesh_comp = (eg_mesh_component_t *)eg_world_add_comp(
        &world, ent, EG_MESH_COMPONENT_TYPE);
    eg_mesh_component_init(mesh_comp, mesh_asset, material);
  }

  {
    eg_entity_t ent = eg_world_add_entity(&world);
    eg_transform_component_t *transform_comp =
        (eg_transform_component_t *)eg_world_add_comp(
            &world, ent, EG_TRANSFORM_COMPONENT_TYPE);
    eg_transform_component_init(transform_comp);
    eg_mesh_component_t *mesh_comp = (eg_mesh_component_t *)eg_world_add_comp(
        &world, ent, EG_MESH_COMPONENT_TYPE);
    eg_mesh_component_init(mesh_comp, mesh_asset, material);
  }

  re_texture_t heightmap;
  re_resource_set_t heightmap_resource_sets[RE_MAX_FRAMES_IN_FLIGHT];

  eg_mesh_component_t heightmap_mesh;
  const uint32_t heightmap_width = 20;
  const uint32_t heightmap_height = 20;

  srand(time(0));
#define RAND(lo, hi) (lo + ((float)rand() / ((float)RAND_MAX / (hi - lo))))
#define VERTEX_ID(i, j) heightmap_height *(i) + (j)

  eg_pbr_material_asset_t *heightmap_material =
      eg_asset_alloc(&asset_manager, eg_pbr_material_asset_t);

  // *------------------------------*
  // | Heightmap texture generation |
  // *------------------------------*
  {
    float heightmap_data[heightmap_width * heightmap_height] = {};

    for (uint32_t i = 0; i < heightmap_width; i++) {
      for (uint32_t j = 0; j < heightmap_height; j++) {
        heightmap_data[VERTEX_ID(i, j)] = RAND(0.0f, 1.0f);
      }
    }

    re_texture_init(
        &heightmap,
        (uint8_t *)heightmap_data,
        sizeof(heightmap_data),
        heightmap_width,
        heightmap_height,
        RE_FORMAT_R32_SFLOAT);

    eg_pbr_material_asset_init(
        heightmap_material, &heightmap, NULL, NULL, NULL, NULL);

    for (uint32_t i = 0; i < RE_MAX_FRAMES_IN_FLIGHT; i++) {
      heightmap_resource_sets[i] = re_allocate_resource_set(
          &g_ctx.resource_manager.set_layouts.single_texture);

      VkWriteDescriptorSet descriptor_writes[] = {
          VkWriteDescriptorSet{
              VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
              NULL,
              heightmap_resource_sets[i].descriptor_set, // dstSet
              0,                                         // dstBinding
              0,                                         // dstArrayElement
              1,                                         // descriptorCount
              VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, // descriptorType
              &heightmap.descriptor,                     // pImageInfo
              NULL,                                      // pBufferInfo
              NULL,                                      // pTexelBufferView
          },
      };

      vkUpdateDescriptorSets(
          g_ctx.device,
          ARRAYSIZE(descriptor_writes),
          descriptor_writes,
          0,
          NULL);
    }
  }

  // *---------------------------*
  // | Heightmap mesh generation |
  // *---------------------------*
  {
    re_vertex_t plane_vertices[heightmap_width * heightmap_height];
    uint32_t plane_indices
        [heightmap_width * heightmap_height * 2 * 3]; // number of triangles * 3

    // Generate initial vertices
    for (uint32_t i = 0; i < heightmap_width; i++) {
      for (uint32_t j = 0; j < heightmap_height; j++) {
        re_vertex_t *vertex = &plane_vertices[VERTEX_ID(i, j)];

        vertex->pos = {(float)i, 0.0, (float)j};
        vertex->normal = {0, 1.0f, 0};
        vertex->uv = {(float)i / (float)(heightmap_width - 1),
                      (float)j / (float)(heightmap_height - 1)};
      }
    }

    // Populate the indices
    uint32_t curr = 0;
    for (uint32_t i = 0; i < heightmap_width - 1; i++) {
      for (uint32_t j = 0; j < heightmap_height - 1; j++) {
        plane_indices[curr++] = VERTEX_ID(i, j);
        plane_indices[curr++] = VERTEX_ID(i, j + 1);
        plane_indices[curr++] = VERTEX_ID(i + 1, j + 1);

        plane_indices[curr++] = VERTEX_ID(i + 1, j + 1);
        plane_indices[curr++] = VERTEX_ID(i + 1, j);
        plane_indices[curr++] = VERTEX_ID(i, j);
      }
    }

    // Create asset
    eg_mesh_asset_t *plane_asset =
        eg_asset_alloc(&asset_manager, eg_mesh_asset_t);
    eg_mesh_asset_init(
        plane_asset,
        plane_vertices,
        ARRAYSIZE(plane_vertices),
        plane_indices,
        ARRAYSIZE(plane_indices));

    // Create heightap mesh
    eg_mesh_component_init(&heightmap_mesh, plane_asset, heightmap_material);
  }

  while (!window.should_close) {
    SDL_Event event;
    while (re_window_poll_event(&window, &event)) {
      re_imgui_process_event(&imgui, &event);
      eg_fps_camera_system_process_event(&fps_system, &window, &event);

      switch (event.type) {
      case SDL_QUIT:
        window.should_close = true;
        break;
      }
    }

    // Per-frame updates
    eg_environment_update(&world.environment, &window);

    re_imgui_begin(&imgui);

    eg_draw_inspector(&world, &asset_manager);

    re_imgui_end(&imgui);

    re_window_begin_frame(&window);

    eg_fps_camera_system_update(&fps_system, &window, &world.camera);

    re_window_begin_render_pass(&window);

    eg_camera_bind(&world.camera, &window, &skybox_pipeline, 0);
    eg_environment_draw_skybox(&world.environment, &window, &skybox_pipeline);

    re_pipeline_bind_graphics(&pbr_pipeline, &window);
    eg_camera_bind(&world.camera, &window, &pbr_pipeline, 0);
    eg_environment_bind(&world.environment, &window, &pbr_pipeline, 4);

    // Draw all meshes
    for (eg_entity_t entity = 0; entity < EG_MAX_ENTITIES; entity++) {
      if (eg_world_has_comp(&world, entity, EG_MESH_COMPONENT_TYPE) &&
          eg_world_has_comp(&world, entity, EG_TRANSFORM_COMPONENT_TYPE)) {
        eg_mesh_component_t *mesh =
            EG_GET_COMP(&world, entity, eg_mesh_component_t);
        eg_transform_component_t *transform =
            EG_GET_COMP(&world, entity, eg_transform_component_t);

        mesh->model.uniform.transform =
            eg_transform_component_to_mat4(transform);
        eg_mesh_component_draw(mesh, &window, &pbr_pipeline);
      }
    }

    // Draw heightmap
    {
      re_pipeline_bind_graphics(&heightmap_pipeline, &window);
      eg_camera_bind(&world.camera, &window, &heightmap_pipeline, 0);
      eg_environment_bind(&world.environment, &window, &heightmap_pipeline, 4);

      uint32_t i = window.current_frame;
      VkCommandBuffer command_buffer =
          re_window_get_current_command_buffer(&window);

      vkCmdBindDescriptorSets(
          command_buffer,
          VK_PIPELINE_BIND_POINT_GRAPHICS,
          heightmap_pipeline.layout,
          5, // firstSet
          1,
          &heightmap_resource_sets[i].descriptor_set,
          0,
          NULL);

      eg_mesh_component_draw(&heightmap_mesh, &window, &heightmap_pipeline);
    }

    re_imgui_draw(&imgui);

    re_window_end_render_pass(&window);

    re_window_end_frame(&window);
  }

  for (uint32_t i = 0; i < RE_MAX_FRAMES_IN_FLIGHT; i++) {
    re_free_resource_set(
        &g_ctx.resource_manager.set_layouts.single_texture,
        &heightmap_resource_sets[i]);
  }

  eg_mesh_component_destroy(&heightmap_mesh);
  re_texture_destroy(&heightmap);

  eg_world_destroy(&world);
  eg_asset_manager_destroy(&asset_manager);

  re_pipeline_destroy(&heightmap_pipeline);
  re_pipeline_destroy(&pbr_pipeline);
  re_pipeline_destroy(&skybox_pipeline);

  re_shader_destroy_compiler();
  re_imgui_destroy(&imgui);
  re_window_destroy(&window);
  re_context_destroy(&g_ctx);

  return 0;
}
