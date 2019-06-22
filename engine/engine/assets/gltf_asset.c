#include "gltf_asset.h"

#include "../asset_manager.h"
#include "../engine.h"
#include "../filesystem.h"
#include "../imgui.h"
#include "../serializer.h"
#include <assert.h>
#include <cgltf.h>
#include <float.h>
#include <fstd_util.h>
#include <renderer/context.h>
#include <renderer/util.h>
#include <stb_image.h>
#include <stdlib.h>
#include <string.h>

void eg_gltf_asset_inspect(eg_gltf_asset_t *model, eg_inspector_t *inspector) {
  igText("Vertex count: %u", model->vertex_count);
  igText("Index count: %u", model->index_count);
  igText("Image count: %u", model->image_count);
  igText("Mesh count: %u", model->mesh_count);

  for (uint32_t j = 0; j < model->material_count; j++) {
    igPushIDInt(j);
    eg_gltf_asset_material_t *material = &model->materials[j];

    if (igCollapsingHeader("Material", ImGuiTreeNodeFlags_DefaultOpen)) {
      igColorEdit4("Color", &material->uniform.base_color_factor.x, 0);
      igDragFloat(
          "Metallic",
          &material->uniform.metallic,
          0.01f,
          0.0f,
          1.0f,
          "%.3f",
          1.0f);
      igDragFloat(
          "Roughness",
          &material->uniform.roughness,
          0.01f,
          0.0f,
          1.0f,
          "%.3f",
          1.0f);
      igColorEdit4("Emissive factor", &material->uniform.emissive_factor.x, 0);

      const float img_size = 128.0f;

#define IMAGE(image, name)                                                     \
  if ((image)) {                                                               \
    igImage(                                                                   \
        (image),                                                               \
        (ImVec2){img_size, img_size},                                          \
        (ImVec2){0.0f, 0.0f},                                                  \
        (ImVec2){1.0f, 1.0f},                                                  \
        (ImVec4){1.0f, 1.0f, 1.0f, 1.0f},                                      \
        (ImVec4){0.0f, 0.0f, 0.0f, 0.0f});                                     \
                                                                               \
    if (igIsItemHovered(0)) igSetTooltip(name);                                \
  }

      IMAGE(material->albedo_texture, "Albedo");
      IMAGE(material->normal_texture, "Normal");
      IMAGE(material->metallic_roughness_texture, "Metallic/roughness");
      IMAGE(material->occlusion_texture, "Occlusion");
      IMAGE(material->emissive_texture, "Emissive");
    }
    igPopID();
  }
}

static void dimensions_init(eg_gltf_asset_dimensions_t *dimensions) {
  dimensions->min    = (vec3_t){FLT_MAX, FLT_MAX, FLT_MAX};
  dimensions->max    = (vec3_t){-FLT_MAX, -FLT_MAX, -FLT_MAX};
  dimensions->size   = (vec3_t){0, 0, 0};
  dimensions->center = (vec3_t){0, 0, 0};
  dimensions->radius = 0.0f;
}

static void material_init(
    eg_gltf_asset_material_t *material,
    re_image_t *albedo_texture,
    re_image_t *normal_texture,
    re_image_t *metallic_roughness_texture,
    re_image_t *occlusion_texture,
    re_image_t *emissive_texture) {
  material->uniform.base_color_factor  = (vec4_t){1.0, 1.0, 1.0, 1.0};
  material->uniform.metallic           = 1.0;
  material->uniform.roughness          = 1.0;
  material->uniform.emissive_factor    = (vec4_t){1.0, 1.0, 1.0, 1.0};
  material->uniform.has_normal_texture = 1;

  material->albedo_texture = albedo_texture;
  if (material->albedo_texture == NULL) {
    material->albedo_texture = &g_eng.white_texture;
  }

  material->normal_texture = normal_texture;
  if (material->normal_texture == NULL) {
    material->uniform.has_normal_texture = 0;
    material->normal_texture             = &g_eng.white_texture;
  }

  material->metallic_roughness_texture = metallic_roughness_texture;
  if (material->metallic_roughness_texture == NULL) {
    material->metallic_roughness_texture = &g_eng.white_texture;
  }

  material->occlusion_texture = occlusion_texture;
  if (material->occlusion_texture == NULL) {
    material->occlusion_texture = &g_eng.white_texture;
  }

  material->emissive_texture = emissive_texture;
  if (material->emissive_texture == NULL) {
    material->emissive_texture = &g_eng.black_texture;
  }
}

static void primitive_init(
    eg_gltf_asset_primitive_t *primitive,
    uint32_t first_index,
    uint32_t index_count,
    eg_gltf_asset_material_t *material) {
  primitive->first_index = first_index;
  primitive->index_count = index_count;
  primitive->material    = material;
  dimensions_init(&primitive->dimensions);
}

static void primitive_set_dimensions(
    eg_gltf_asset_primitive_t *primitive, vec3_t min, vec3_t max) {
  primitive->dimensions.min    = min;
  primitive->dimensions.max    = max;
  primitive->dimensions.size   = vec3_sub(max, min);
  primitive->dimensions.center = vec3_divs(vec3_add(max, min), 2.0f);
  primitive->dimensions.radius = vec3_distance(min, max) / 2.0f;
}

static void mesh_init(eg_gltf_asset_mesh_t *mesh, mat4_t matrix) {
  mesh->primitives      = NULL;
  mesh->primitive_count = 0;

  mesh->matrix = mat4_identity();
}

static void mesh_destroy(eg_gltf_asset_mesh_t *mesh) {
  if (mesh->primitives != NULL) {
    free(mesh->primitives);
  }
}

static void node_init(eg_gltf_asset_node_t *node) {
  node->parent         = NULL;
  node->children       = NULL;
  node->children_count = 0;
  node->matrix         = mat4_identity();
  node->name           = NULL;
  node->mesh           = NULL;
  node->translation    = (vec3_t){0.0, 0.0, 0.0};
  node->scale          = (vec3_t){1.0, 1.0, 1.0};
  node->rotation       = quat_from_axis_angle((vec3_t){1.0, 0.0, 0.0}, 0.0);
}

static mat4_t node_local_matrix(eg_gltf_asset_node_t *node) {
  return mat4_mul(
      mat4_mul(
          mat4_mul(
              mat4_translate(mat4_identity(), node->translation),
              quat_to_mat4(node->rotation)),
          mat4_scale(mat4_identity(), node->scale)),
      node->matrix);
}

static mat4_t node_get_matrix(eg_gltf_asset_node_t *node) {
  mat4_t m                     = node_local_matrix(node);
  eg_gltf_asset_node_t *parent = node->parent;
  while (parent != NULL) {
    m      = mat4_mul(node_local_matrix(parent), m);
    parent = parent->parent;
  }
  return m;
}

static void node_update(
    eg_gltf_asset_node_t *node, eg_gltf_asset_t *model, uint32_t frame_index) {
  if (node->mesh != NULL) {
    node->mesh->matrix = node_get_matrix(node);
  }

  for (uint32_t i = 0; i < node->children_count; i++) {
    if (node->children[i] != NULL) {
      node_update(node->children[i], model, frame_index);
    }
  }
}

static void node_destroy(eg_gltf_asset_node_t *node) {
  if (node->children != NULL) {
    free(node->children);
  }

  if (node->name != NULL) {
    free(node->name);
  }
}

static void load_node(
    eg_gltf_asset_t *model,
    cgltf_node *parent,
    cgltf_node *node,
    cgltf_data *data,
    re_vertex_t **vertices,
    uint32_t *vertex_count,
    uint32_t **indices,
    uint32_t *index_count,
    bool flip_uvs) {
  eg_gltf_asset_node_t *new_node = &model->nodes[node - data->nodes];
  node_init(new_node);
  if (parent != NULL) {
    new_node->parent = &model->nodes[parent - data->nodes];
  }
  if (node->name) {
    new_node->name = malloc(strlen(node->name) + 1);
    strcpy(new_node->name, node->name);
  }
  new_node->matrix = mat4_identity();

  if (node->has_translation) {
    memcpy(
        &new_node->translation, node->translation, sizeof(node->translation));
  }

  if (node->has_rotation) {
    memcpy(&new_node->rotation, node->rotation, sizeof(node->rotation));
  }

  if (node->has_scale) {
    memcpy(&new_node->scale, node->scale, sizeof(node->scale));
  }

  if (node->has_matrix) {
    memcpy(&new_node->matrix, node->matrix, sizeof(node->matrix));
  }

  if (node->children_count > 0) {
    for (uint32_t i = 0; i < node->children_count; i++) {
      load_node(
          model,
          node,
          node->children[i],
          data,
          vertices,
          vertex_count,
          indices,
          index_count,
          flip_uvs);
    }
  }

  if (node->mesh != NULL) {
    cgltf_mesh *mesh               = node->mesh;
    eg_gltf_asset_mesh_t *new_mesh = &model->meshes[node->mesh - data->meshes];
    mesh_init(new_mesh, new_node->matrix);

    new_mesh->primitive_count = (uint32_t)mesh->primitives_count;
    new_mesh->primitives =
        calloc(new_mesh->primitive_count, sizeof(*new_mesh->primitives));

    for (uint32_t i = 0; i < mesh->primitives_count; i++) {
      cgltf_primitive *primitive = &mesh->primitives[i];

      if (primitive->indices == NULL) {
        continue;
      }

      uint32_t index_start      = *index_count;
      uint32_t vertex_start     = *vertex_count;
      uint32_t prim_index_count = 0;

      vec3_t pos_min = {0};
      vec3_t pos_max = {0};

      // Vertices
      {
        float *buffer_pos       = NULL;
        float *buffer_normals   = NULL;
        float *buffer_texcoords = NULL;

        cgltf_accessor *pos_accessor      = NULL;
        cgltf_accessor *normal_accessor   = NULL;
        cgltf_accessor *texcoord_accessor = NULL;
        for (uint32_t j = 0; j < primitive->attributes_count; j++) {
          if (primitive->attributes[j].type == cgltf_attribute_type_position) {
            pos_accessor = primitive->attributes[j].data;
          }
          if (primitive->attributes[j].type == cgltf_attribute_type_texcoord) {
            texcoord_accessor = primitive->attributes[j].data;
          }
          if (primitive->attributes[j].type == cgltf_attribute_type_normal) {
            normal_accessor = primitive->attributes[j].data;
          }
        }

        assert(pos_accessor != NULL);

        cgltf_buffer_view *pos_view = pos_accessor->buffer_view;
        buffer_pos =
            (float *)&((unsigned char *)pos_view->buffer
                           ->data)[pos_accessor->offset + pos_view->offset];

        if (pos_accessor->has_min) {
          pos_min = (vec3_t){
              pos_accessor->min[0],
              pos_accessor->min[1],
              pos_accessor->min[2],
          };
        }

        if (pos_accessor->has_max) {
          pos_max = (vec3_t){
              pos_accessor->max[0],
              pos_accessor->max[1],
              pos_accessor->max[2],
          };
        }

        if (normal_accessor != NULL) {
          cgltf_buffer_view *normal_view = normal_accessor->buffer_view;
          buffer_normals                 = (float *)&(
              (unsigned char *)normal_view->buffer
                  ->data)[normal_accessor->offset + normal_view->offset];
        }

        if (texcoord_accessor != NULL) {
          cgltf_buffer_view *texcoord_view = texcoord_accessor->buffer_view;
          buffer_texcoords                 = (float *)&(
              ((unsigned char *)texcoord_view->buffer
                   ->data)[texcoord_accessor->offset + texcoord_view->offset]);
        }

        *vertex_count += (uint32_t)pos_accessor->count;
        *vertices = realloc(*vertices, (*vertex_count) * sizeof(re_vertex_t));

        for (uint32_t v = 0; v < pos_accessor->count; v++) {
          re_vertex_t vert = {0};

          memcpy(&vert.pos, &buffer_pos[v * 3], sizeof(float) * 3);
          if (normal_accessor != NULL) {
            memcpy(&vert.normal, &buffer_normals[v * 3], sizeof(float) * 3);
          }
          if (texcoord_accessor != NULL) {
            memcpy(&vert.uv, &buffer_texcoords[v * 2], sizeof(float) * 2);
          }

          if (flip_uvs) {
            vert.uv.y = 1.0f - vert.uv.y;
          }

          (*vertices)[(*vertex_count) - pos_accessor->count + v] = vert;
        }
      }

      // Indices
      {
        cgltf_accessor *accessor       = primitive->indices;
        cgltf_buffer_view *buffer_view = accessor->buffer_view;
        cgltf_buffer *buffer           = buffer_view->buffer;

        prim_index_count = (uint32_t)accessor->count;

        *index_count += prim_index_count;
        *indices = realloc(*indices, (*index_count) * sizeof(**indices));

        switch (accessor->component_type) {
        case cgltf_component_type_r_32u: {
          uint32_t *buf = calloc(accessor->count, sizeof(*buf));
          memcpy(
              buf,
              &((unsigned char *)
                    buffer->data)[accessor->offset + buffer_view->offset],
              accessor->count * sizeof(*buf));
          for (size_t index = 0; index < accessor->count; index++) {
            (*indices)[(*index_count) - prim_index_count + index] =
                buf[index] + vertex_start;
          }
          free(buf);
          break;
        }
        case cgltf_component_type_r_16u: {
          uint16_t *buf = calloc(accessor->count, sizeof(*buf));
          memcpy(
              buf,
              &((unsigned char *)
                    buffer->data)[accessor->offset + buffer_view->offset],
              accessor->count * sizeof(*buf));
          for (size_t index = 0; index < accessor->count; index++) {
            (*indices)[(*index_count) - prim_index_count + index] =
                buf[index] + vertex_start;
          }
          free(buf);
          break;
        }
        case cgltf_component_type_r_8u: {
          uint8_t *buf = calloc(accessor->count, sizeof(*buf));
          memcpy(
              buf,
              &((unsigned char *)
                    buffer->data)[accessor->offset + buffer_view->offset],
              accessor->count * sizeof(*buf));
          for (size_t index = 0; index < accessor->count; index++) {
            (*indices)[(*index_count) - prim_index_count + index] =
                buf[index] + vertex_start;
          }
          free(buf);
          break;
        }
        default: {
          assert(false);
        }
        }

        eg_gltf_asset_primitive_t new_primitive;
        primitive_init(
            &new_primitive,
            index_start,
            prim_index_count,
            &model->materials[primitive->material - data->materials]);
        primitive_set_dimensions(&new_primitive, pos_min, pos_max);
        new_mesh->primitives[i] = new_primitive;
      }

      if (node->mesh != NULL) {
        new_node->mesh = &model->meshes[node->mesh - data->meshes];
      }
    }
  }
}

static void
get_node_dimensions(eg_gltf_asset_node_t *node, vec3_t *min, vec3_t *max) {
  if (node->mesh != NULL) {
    for (uint32_t i = 0; i < node->mesh->primitive_count; i++) {
      eg_gltf_asset_primitive_t *primitive = &node->mesh->primitives[i];
      vec4_t loc_min                       = mat4_mulv(
          node_get_matrix(node),
          (vec4_t){.xyz = primitive->dimensions.min, .w = 1.0f});
      vec4_t loc_max = mat4_mulv(
          node_get_matrix(node),
          (vec4_t){.xyz = primitive->dimensions.max, .w = 1.0f});

      if (loc_min.x < min->x) {
        min->x = loc_min.x;
      }
      if (loc_min.y < min->y) {
        min->y = loc_min.y;
      }
      if (loc_min.z < min->z) {
        min->z = loc_min.z;
      }
      if (loc_max.x > max->x) {
        max->x = loc_max.x;
      }
      if (loc_max.y > max->y) {
        max->y = loc_max.y;
      }
      if (loc_max.z > max->z) {
        max->z = loc_max.z;
      }
    }
  }

  for (uint32_t i = 0; i < node->children_count; i++) {
    get_node_dimensions(node->children[i], min, max);
  }
}

static void get_scene_dimensions(eg_gltf_asset_t *model) {
  model->dimensions.min = (vec3_t){FLT_MAX, FLT_MAX, FLT_MAX};
  model->dimensions.max = (vec3_t){-FLT_MAX, -FLT_MAX, -FLT_MAX};
  for (uint32_t i = 0; i < model->node_count; i++) {
    get_node_dimensions(
        &model->nodes[i], &model->dimensions.min, &model->dimensions.max);
  }
  model->dimensions.size =
      vec3_sub(model->dimensions.max, model->dimensions.min);
  model->dimensions.center =
      vec3_divs(vec3_add(model->dimensions.max, model->dimensions.min), 2.0f);
  model->dimensions.radius =
      vec3_distance(model->dimensions.min, model->dimensions.max) / 2.0f;
}

eg_gltf_asset_t *eg_gltf_asset_create(
    eg_asset_manager_t *asset_manager, eg_gltf_asset_options_t *options) {

  eg_gltf_asset_t *model =
      eg_asset_manager_alloc(asset_manager, EG_ASSET_TYPE(eg_gltf_asset_t));

  model->path     = strdup(options->path);
  model->flip_uvs = options->flip_uvs;

  model->vertex_buffer = (re_buffer_t){0};
  model->index_buffer  = (re_buffer_t){0};

  model->images      = NULL;
  model->image_count = 0;

  model->materials      = NULL;
  model->material_count = 0;

  model->nodes      = NULL;
  model->node_count = 0;

  model->meshes     = NULL;
  model->mesh_count = 0;

  dimensions_init(&model->dimensions);

  eg_file_t *gltf_file = eg_file_open_read(options->path);
  assert(gltf_file);
  size_t gltf_size = eg_file_size(gltf_file);
  assert(gltf_size > 0);
  unsigned char *gltf_data = calloc(1, gltf_size);
  assert(gltf_data);
  eg_file_read_bytes(gltf_file, gltf_data, gltf_size);
  eg_file_close(gltf_file);

  cgltf_options gltf_options = {0};
  cgltf_data *data           = NULL;
  cgltf_result result = cgltf_parse(&gltf_options, gltf_data, gltf_size, &data);
  assert(result == cgltf_result_success);

  result = cgltf_load_buffers(&gltf_options, data, options->path);
  assert(result == cgltf_result_success);

  assert(data->file_type == cgltf_file_type_glb);

  // Load images
  model->image_count = (uint32_t)data->images_count;
  model->images      = calloc(model->image_count, sizeof(*model->images));
  for (uint32_t i = 0; i < model->image_count; i++) {
    cgltf_image *image = &data->images[i];
    unsigned char *buffer_data =
        &(((unsigned char *)
               image->buffer_view->buffer->data)[image->buffer_view->offset]);
    cgltf_size buffer_size = image->buffer_view->size;

    int width, height, n_channels;
    unsigned char *image_data = stbi_load_from_memory(
        buffer_data, (int)buffer_size, &width, &height, &n_channels, 4);

    assert(image_data != NULL);

    re_image_options_t image_options = {
        .width           = (uint32_t)width,
        .height          = (uint32_t)height,
        .layer_count     = 1,
        .mip_level_count = 1,
        .format          = VK_FORMAT_R8G8B8A8_UNORM,
        .flags           = RE_IMAGE_FLAG_ANISOTROPY,
        .usage           = RE_IMAGE_USAGE_SAMPLED | RE_IMAGE_USAGE_TRANSFER_DST,
    };

    re_image_init(&model->images[i], &image_options);

    re_image_upload(
        &model->images[i],
        g_ctx.transient_command_pool,
        image_data,
        (uint32_t)width,
        (uint32_t)height,
        0,
        0);

    free(image_data);
  }

  // Load materials
  model->material_count = (uint32_t)data->materials_count;
  model->materials = calloc(model->material_count, sizeof(*model->materials));
  for (uint32_t i = 0; i < data->materials_count; i++) {
    cgltf_material *material = &data->materials[i];
    assert(material->has_pbr_metallic_roughness);

    re_image_t *albedo_image             = NULL;
    re_image_t *normal_image             = NULL;
    re_image_t *metallic_roughness_image = NULL;
    re_image_t *occlusion_image          = NULL;
    re_image_t *emissive_image           = NULL;

    if (material->pbr_metallic_roughness.base_color_texture.texture != NULL) {
      albedo_image = &model->images
                          [material->pbr_metallic_roughness.base_color_texture
                               .texture->image -
                           data->images];
    }

    if (material->normal_texture.texture != NULL) {
      normal_image =
          &model
               ->images[material->normal_texture.texture->image - data->images];
    }

    if (material->pbr_metallic_roughness.metallic_roughness_texture.texture !=
        NULL) {
      metallic_roughness_image =
          &model->images
               [material->pbr_metallic_roughness.metallic_roughness_texture
                    .texture->image -
                data->images];
    }

    if (material->occlusion_texture.texture != NULL) {
      occlusion_image =
          &model->images
               [material->occlusion_texture.texture->image - data->images];
    }

    if (material->emissive_texture.texture != NULL) {
      emissive_image =
          &model->images
               [material->emissive_texture.texture->image - data->images];
    }

    material_init(
        &model->materials[i],
        albedo_image,
        normal_image,
        metallic_roughness_image,
        occlusion_image,
        emissive_image);
  }

  // Nodes and meshes
  model->node_count = (uint32_t)data->nodes_count;
  model->nodes      = calloc(model->node_count, sizeof(*model->nodes));

  model->mesh_count = (uint32_t)data->meshes_count;
  model->meshes     = calloc(model->mesh_count, sizeof(*model->meshes));

  re_vertex_t *vertices = NULL;
  model->vertex_count   = 0;
  uint32_t *indices     = NULL;
  model->index_count    = 0;

  for (size_t i = 0; i < data->scene->nodes_count; i++) {
    load_node(
        model,
        NULL,
        data->scene->nodes[i],
        data,
        &vertices,
        &model->vertex_count,
        &indices,
        &model->index_count,
        options->flip_uvs);
  }

  for (size_t i = 0; i < data->scene->nodes_count; i++) {
    for (uint32_t j = 0; j < RE_MAX_FRAMES_IN_FLIGHT; j++) {
      node_update(&model->nodes[i], model, j);
    }
  }

  // Vertex and index buffers
  size_t vertex_buffer_size = model->vertex_count * sizeof(re_vertex_t);
  size_t index_buffer_size  = model->index_count * sizeof(uint32_t);

  assert((vertex_buffer_size > 0) && (index_buffer_size > 0));

  re_buffer_t staging_buffer;
  re_buffer_init(
      &staging_buffer,
      &(re_buffer_options_t){
          .usage  = RE_BUFFER_USAGE_TRANSFER,
          .memory = RE_BUFFER_MEMORY_HOST,
          .size   = vertex_buffer_size > index_buffer_size ? vertex_buffer_size
                                                         : index_buffer_size,
      });

  void *staging_memory_ptr;
  re_buffer_map_memory(&staging_buffer, &staging_memory_ptr);

  re_buffer_init(
      &model->vertex_buffer,
      &(re_buffer_options_t){
          .usage  = RE_BUFFER_USAGE_VERTEX,
          .memory = RE_BUFFER_MEMORY_DEVICE,
          .size   = vertex_buffer_size,
      });

  re_buffer_init(
      &model->index_buffer,
      &(re_buffer_options_t){
          .usage  = RE_BUFFER_USAGE_INDEX,
          .memory = RE_BUFFER_MEMORY_DEVICE,
          .size   = index_buffer_size,
      });

  memcpy(staging_memory_ptr, vertices, vertex_buffer_size);
  re_buffer_transfer_to_buffer(
      &staging_buffer,
      &model->vertex_buffer,
      g_ctx.transient_command_pool,
      vertex_buffer_size);

  memcpy(staging_memory_ptr, indices, index_buffer_size);
  re_buffer_transfer_to_buffer(
      &staging_buffer,
      &model->index_buffer,
      g_ctx.transient_command_pool,
      index_buffer_size);

  re_buffer_unmap_memory(&staging_buffer);
  re_buffer_destroy(&staging_buffer);

  // TODO: get scene dimensions
  get_scene_dimensions(model);

  cgltf_free(data);
  free(gltf_data);

  if (vertices != NULL) {
    free(vertices);
  }
  if (indices != NULL) {
    free(indices);
  }

  return model;
}

void eg_gltf_asset_destroy(eg_gltf_asset_t *model) {
  re_buffer_destroy(&model->vertex_buffer);
  re_buffer_destroy(&model->index_buffer);

  for (uint32_t i = 0; i < model->mesh_count; i++) {
    mesh_destroy(&model->meshes[i]);
  }
  free(model->meshes);

  for (uint32_t i = 0; i < model->node_count; i++) {
    node_destroy(&model->nodes[i]);
  }
  free(model->nodes);

  for (uint32_t i = 0; i < model->image_count; i++) {
    re_image_destroy(&model->images[i]);
  }
  free(model->images);

  free(model->materials);

  if (model->path) {
    free(model->path);
  }
}

enum {
  PROP_FLIP_UVS,
  PROP_PATH,
  PROP_MAX,
};

void eg_gltf_asset_serialize(
    eg_gltf_asset_t *model, eg_serializer_t *serializer) {
  eg_serializer_append_u32(serializer, PROP_MAX);

  eg_serializer_append_u32(serializer, PROP_FLIP_UVS);
  eg_serializer_append(serializer, &model->flip_uvs, sizeof(model->flip_uvs));

  eg_serializer_append_u32(serializer, PROP_PATH);
  eg_serializer_append_string(serializer, model->path);
}
