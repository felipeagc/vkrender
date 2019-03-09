#include "mesh_component.h"
#include "../assets/mesh_asset.h"

void eg_mesh_component_init(
    eg_mesh_component_t *mesh,
    struct eg_mesh_asset_t *asset,
    eg_pbr_material_asset_t *material) {
  eg_pbr_model_init(&mesh->model, mat4_identity());
  eg_pbr_model_init(&mesh->local_model, mat4_identity());

  mesh->asset = asset;
  mesh->material = material;
}

void eg_mesh_component_draw(
    eg_mesh_component_t *mesh,
    struct re_window_t *window,
    struct re_pipeline_t *pipeline) {
  eg_pbr_material_asset_bind(mesh->material, window, pipeline, 4);
  eg_pbr_model_update_uniform(&mesh->local_model, window);
  eg_pbr_model_update_uniform(&mesh->model, window);
  eg_pbr_model_bind(&mesh->local_model, window, pipeline, 2);
  eg_pbr_model_bind(&mesh->model, window, pipeline, 3);
  eg_mesh_asset_draw(mesh->asset, window);
}

void eg_mesh_component_destroy(eg_mesh_component_t *mesh) {
  eg_pbr_model_destroy(&mesh->model);
  eg_pbr_model_destroy(&mesh->local_model);
}
