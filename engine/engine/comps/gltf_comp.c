#include "gltf_comp.h"

#include "../assets/gltf_asset.h"
#include "../inspector_utils.h"
#include "../pipelines.h"
#include "../serializer.h"
#include <fstd_util.h>
#include <renderer/context.h>
#include <renderer/pipeline.h>
#include <renderer/util.h>
#include <renderer/window.h>
#include <string.h>

void eg_gltf_comp_default(eg_gltf_comp_t *model) { model->asset = NULL; }

void eg_gltf_comp_inspect(eg_gltf_comp_t *model, eg_inspector_t *inspector) {
  eg_inspect_assets(
      inspector,
      model,
      1,
      (eg_inspect_assets_t[]){
          {"Model",
           (eg_asset_t **)&model->asset,
           EG_ASSET_TYPE(eg_gltf_asset_t)},
      });
}

void eg_gltf_comp_destroy(eg_gltf_comp_t *model) {}

void eg_gltf_comp_serialize(
    eg_gltf_comp_t *model, eg_serializer_t *serializer) {
  eg_asset_uid_t uid = EG_NULL_ASSET_UID;
  if (model->asset) uid = model->asset->asset.uid;

  eg_serializer_append(serializer, &uid, sizeof(uid));
}

static void draw_node(
    eg_gltf_comp_t *model,
    eg_gltf_asset_node_t *node,
    re_cmd_buffer_t *cmd_buffer,
    re_pipeline_t *pipeline,
    mat4_t matrix) {
  if (node->mesh != NULL) {

    // Mesh
    {
      struct {
        mat4_t local_model;
        mat4_t model;
      } ubo;

      ubo.local_model = node->mesh->matrix;
      ubo.model       = matrix;

      void *mapping = re_cmd_bind_uniform(cmd_buffer, 2, 0, sizeof(ubo));
      memcpy(mapping, &ubo, sizeof(ubo));

      re_cmd_bind_descriptor_set(cmd_buffer, pipeline, 2);
    }

    for (uint32_t j = 0; j < node->mesh->primitive_count; j++) {
      eg_gltf_asset_primitive_t *primitive = &node->mesh->primitives[j];

      if (primitive->material != NULL) {
        // Material
        {

          re_cmd_bind_image(
              cmd_buffer, 3, 0, primitive->material->albedo_texture);
          re_cmd_bind_image(
              cmd_buffer, 3, 1, primitive->material->normal_texture);
          re_cmd_bind_image(
              cmd_buffer,
              3,
              2,
              primitive->material->metallic_roughness_texture);
          re_cmd_bind_image(
              cmd_buffer, 3, 3, primitive->material->occlusion_texture);
          re_cmd_bind_image(
              cmd_buffer, 3, 4, primitive->material->emissive_texture);

          void *mapping = re_cmd_bind_uniform(
              cmd_buffer, 3, 5, sizeof(primitive->material->uniform));
          memcpy(
              mapping,
              &primitive->material->uniform,
              sizeof(primitive->material->uniform));

          re_cmd_bind_descriptor_set(cmd_buffer, pipeline, 3);
        }

        re_cmd_draw_indexed(
            cmd_buffer,
            primitive->index_count,
            1,
            primitive->first_index,
            0,
            0);
      }
    }
  }

  for (uint32_t j = 0; j < node->children_count; j++) {
    draw_node(model, node->children[j], cmd_buffer, pipeline, matrix);
  }
}

static void draw_node_no_mat(
    eg_gltf_comp_t *model,
    eg_gltf_asset_node_t *node,
    re_cmd_buffer_t *cmd_buffer,
    re_pipeline_t *pipeline,
    mat4_t matrix) {
  if (node->mesh != NULL) {
    // Mesh
    {
      struct {
        mat4_t local_model;
        mat4_t model;
      } ubo;

      ubo.local_model = node->mesh->matrix;
      ubo.model       = matrix;

      void *mapping = re_cmd_bind_uniform(cmd_buffer, 1, 0, sizeof(ubo));
      memcpy(mapping, &ubo, sizeof(ubo));

      re_cmd_bind_descriptor_set(cmd_buffer, pipeline, 1);
    }

    for (uint32_t j = 0; j < node->mesh->primitive_count; j++) {
      eg_gltf_asset_primitive_t *primitive = &node->mesh->primitives[j];

      if (primitive->material != NULL) {
        re_cmd_draw_indexed(
            cmd_buffer,
            primitive->index_count,
            1,
            primitive->first_index,
            0,
            0);
      }
    }
  }

  for (uint32_t j = 0; j < node->children_count; j++) {
    draw_node_no_mat(model, node->children[j], cmd_buffer, pipeline, matrix);
  }
}

void eg_gltf_comp_init(eg_gltf_comp_t *model, eg_gltf_asset_t *asset) {
  model->asset = asset;
}

void eg_gltf_comp_draw(
    eg_gltf_comp_t *model,
    re_cmd_buffer_t *cmd_buffer,
    re_pipeline_t *pipeline,
    mat4_t transform) {
  if (!model->asset) return;

  size_t offset = 0;
  re_cmd_bind_vertex_buffers(
      cmd_buffer, 0, 1, &model->asset->vertex_buffer, &offset);

  re_cmd_bind_index_buffer(
      cmd_buffer, &model->asset->index_buffer, 0, RE_INDEX_TYPE_UINT32);

  for (uint32_t j = 0; j < model->asset->node_count; j++) {
    draw_node(model, &model->asset->nodes[j], cmd_buffer, pipeline, transform);
  }
}

void eg_gltf_comp_draw_no_mat(
    eg_gltf_comp_t *model,
    re_cmd_buffer_t *cmd_buffer,
    re_pipeline_t *pipeline,
    mat4_t transform) {
  if (!model->asset) return;

  size_t offset = 0;
  re_cmd_bind_vertex_buffers(
      cmd_buffer, 0, 1, &model->asset->vertex_buffer, &offset);

  re_cmd_bind_index_buffer(
      cmd_buffer, &model->asset->index_buffer, 0, RE_INDEX_TYPE_UINT32);

  for (uint32_t j = 0; j < model->asset->node_count; j++) {
    draw_node_no_mat(
        model, &model->asset->nodes[j], cmd_buffer, pipeline, transform);
  }
}
