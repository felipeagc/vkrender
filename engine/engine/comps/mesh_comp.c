#include "mesh_comp.h"

#include "../assets/mesh_asset.h"
#include <renderer/context.h>
#include <string.h>

void eg_mesh_comp_init(
    eg_mesh_comp_t *mesh,
    struct eg_mesh_asset_t *asset,
    eg_pbr_material_asset_t *material) {
  mesh->uniform.model = mat4_identity();
  mesh->uniform.local_model = mat4_identity();

  mesh->asset = asset;
  mesh->material = material;
}

void eg_mesh_comp_draw(
    eg_mesh_comp_t *mesh,
    re_cmd_buffer_t *cmd_buffer,
    struct re_pipeline_t *pipeline) {
  eg_pbr_material_asset_bind(mesh->material, cmd_buffer, pipeline, 3);

  void *mapping = re_cmd_bind_uniform(cmd_buffer, 0, sizeof(mesh->uniform));
  memcpy(mapping, &mesh->uniform, sizeof(mesh->uniform));

  re_cmd_bind_descriptor_set(cmd_buffer, pipeline, 2);

  eg_mesh_asset_draw(mesh->asset, cmd_buffer);
}

void eg_mesh_comp_draw_no_mat(
    eg_mesh_comp_t *mesh,
    re_cmd_buffer_t *cmd_buffer,
    struct re_pipeline_t *pipeline) {
  void *mapping = re_cmd_bind_uniform(cmd_buffer, 0, sizeof(mesh->uniform));
  memcpy(mapping, &mesh->uniform, sizeof(mesh->uniform));

  re_cmd_bind_descriptor_set(cmd_buffer, pipeline, 1);

  eg_mesh_asset_draw(mesh->asset, cmd_buffer);
}

void eg_mesh_comp_destroy(eg_mesh_comp_t *mesh) {}
