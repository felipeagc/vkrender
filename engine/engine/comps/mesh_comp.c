#include "mesh_comp.h"

#include "../assets/mesh_asset.h"
#include "../assets/pbr_material_asset.h"
#include "../deserializer.h"
#include "../inspector_utils.h"
#include "../serializer.h"
#include <renderer/context.h>
#include <string.h>

void eg_mesh_comp_default(eg_mesh_comp_t *mesh) {
  mesh->asset    = NULL;
  mesh->material = NULL;
}

void eg_mesh_comp_inspect(eg_mesh_comp_t *mesh, eg_inspector_t *inspector) {
  eg_inspect_assets(
      inspector,
      mesh,
      2,
      (eg_inspect_assets_t[]){
          {"Material",
           (eg_asset_t **)&mesh->material,
           EG_ASSET_TYPE(eg_pbr_material_asset_t)},
          {"Mesh", (eg_asset_t **)&mesh->asset, EG_ASSET_TYPE(eg_mesh_asset_t)},
      });
}

void eg_mesh_comp_destroy(eg_mesh_comp_t *mesh) {}

enum {
  PROP_MATERIAL,
  PROP_MESH,
  PROP_MAX,
};

void eg_mesh_comp_serialize(eg_mesh_comp_t *mesh, eg_serializer_t *serializer) {
  eg_serializer_append_u32(serializer, PROP_MAX);

  eg_asset_uid_t material_uid = EG_NULL_ASSET_UID;
  if (mesh->material) material_uid = mesh->material->asset.uid;

  eg_asset_uid_t mesh_uid = EG_NULL_ASSET_UID;
  if (mesh->asset) mesh_uid = mesh->asset->asset.uid;

  eg_serializer_append_u32(serializer, PROP_MATERIAL);
  eg_serializer_append_u32(serializer, material_uid);

  eg_serializer_append_u32(serializer, PROP_MESH);
  eg_serializer_append_u32(serializer, mesh_uid);
}

void eg_mesh_comp_deserialize(
    eg_mesh_comp_t *mesh, eg_deserializer_t *deserializer) {
  uint32_t prop_count = eg_deserializer_read_u32(deserializer);

  for (uint32_t i = 0; i < prop_count; i++) {
    uint32_t prop = eg_deserializer_read_u32(deserializer);

    switch (prop) {
    case PROP_MATERIAL: {
      eg_asset_uid_t uid = eg_deserializer_read_u32(deserializer);
      mesh->material =
          eg_asset_manager_get_by_uid(deserializer->asset_manager, uid);
      break;
    }
    case PROP_MESH: {
      eg_asset_uid_t uid = eg_deserializer_read_u32(deserializer);
      mesh->asset =
          eg_asset_manager_get_by_uid(deserializer->asset_manager, uid);
      break;
    }
    default: break;
    }
  }
}

void eg_mesh_comp_init(
    eg_mesh_comp_t *mesh,
    eg_mesh_asset_t *asset,
    eg_pbr_material_asset_t *material) {
  mesh->asset    = asset;
  mesh->material = material;
}

void eg_mesh_comp_draw(
    eg_mesh_comp_t *mesh,
    re_cmd_buffer_t *cmd_buffer,
    re_pipeline_t *pipeline,
    mat4_t transform) {
  if (mesh->material == NULL) return;
  if (mesh->asset == NULL) return;

  eg_pbr_material_asset_bind(mesh->material, cmd_buffer, pipeline, 3);

  struct {
    mat4_t model;
    mat4_t local_model;
  } uniform;

  uniform.local_model = mat4_identity();
  uniform.model       = transform;

  void *mapping = re_cmd_bind_uniform(cmd_buffer, 2, 0, sizeof(uniform));
  memcpy(mapping, &uniform, sizeof(uniform));

  re_cmd_bind_descriptor_set(cmd_buffer, pipeline, 2);

  eg_mesh_asset_draw(mesh->asset, cmd_buffer);
}

void eg_mesh_comp_draw_no_mat(
    eg_mesh_comp_t *mesh,
    re_cmd_buffer_t *cmd_buffer,
    re_pipeline_t *pipeline,
    mat4_t transform) {
  if (mesh->asset == NULL) return;

  struct {
    mat4_t model;
    mat4_t local_model;
  } uniform;

  uniform.local_model = mat4_identity();
  uniform.model       = transform;

  void *mapping = re_cmd_bind_uniform(cmd_buffer, 1, 0, sizeof(uniform));
  memcpy(mapping, &uniform, sizeof(uniform));

  re_cmd_bind_descriptor_set(cmd_buffer, pipeline, 1);

  eg_mesh_asset_draw(mesh->asset, cmd_buffer);
}
