#include "mesh_comp.h"
#include "../assets/mesh_asset.h"

void eg_mesh_comp_init(
    eg_mesh_comp_t *mesh,
    struct eg_mesh_asset_t *asset,
    eg_pbr_material_asset_t *material) {
  eg_pbr_model_init(&mesh->model, mat4_identity());
  eg_pbr_model_init(&mesh->local_model, mat4_identity());

  mesh->asset = asset;
  mesh->material = material;
}

void eg_mesh_comp_draw(
    eg_mesh_comp_t *mesh,
    const eg_cmd_info_t *cmd_info,
    struct re_pipeline_t *pipeline) {
  eg_pbr_material_asset_bind(mesh->material, cmd_info, pipeline, 4);
  eg_pbr_model_update_uniform(&mesh->local_model, cmd_info);
  eg_pbr_model_update_uniform(&mesh->model, cmd_info);
  eg_pbr_model_bind(&mesh->local_model, cmd_info, pipeline, 2);
  eg_pbr_model_bind(&mesh->model, cmd_info, pipeline, 3);
  eg_mesh_asset_draw(mesh->asset, cmd_info);
}

void eg_mesh_comp_destroy(eg_mesh_comp_t *mesh) {
  eg_pbr_model_destroy(&mesh->model);
  eg_pbr_model_destroy(&mesh->local_model);
}
