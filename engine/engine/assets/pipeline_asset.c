#include "pipeline_asset.h"

#include "../asset_manager.h"
#include "../deserializer.h"
#include "../filesystem.h"
#include "../imgui.h"
#include "../pipelines.h"
#include "../serializer.h"

void eg_pipeline_asset_init(
    eg_pipeline_asset_t *pipeline_asset, eg_pipeline_asset_options_t *options) {
  pipeline_asset->vert_path = strdup(options->vert_path);
  pipeline_asset->frag_path = strdup(options->frag_path);
  pipeline_asset->params    = options->params;

  eg_init_pipeline_spv(
      &pipeline_asset->pipeline,
      (const char *[]){options->vert_path, options->frag_path},
      2,
      options->params);
}

void eg_pipeline_asset_inspect(
    eg_pipeline_asset_t *pipeline_asset, eg_inspector_t *inspector) {
  static const float indent = 8.0f;

  const re_pipeline_layout_t *const layout = &pipeline_asset->pipeline.layout;
  for (uint32_t i = 0; i < layout->descriptor_set_count; i++) {
    re_descriptor_set_allocator_t *allocator =
        layout->descriptor_set_allocators[i];

    igText("Set %u (%u bindings):", i, allocator->binding_count);

    igIndent(indent);
    for (uint32_t b = 0; b < RE_MAX_DESCRIPTOR_SET_BINDINGS; b++) {
      if (allocator->layout.combined_image_sampler_mask & (1 << b)) {
        igText("> Binding %u: combined image sampler", b);
      }

      if (allocator->layout.uniform_buffer_dynamic_mask & (1 << b)) {
        igText("> Binding %u: uniform buffer dynamic", b);
      }
    }
    igUnindent(indent);
  }
}

void eg_pipeline_asset_destroy(eg_pipeline_asset_t *pipeline_asset) {
  re_pipeline_destroy(&pipeline_asset->pipeline);

  if (pipeline_asset->vert_path) free(pipeline_asset->vert_path);
  if (pipeline_asset->frag_path) free(pipeline_asset->frag_path);
}

enum {
  PROP_VERT_PATH,
  PROP_FRAG_PATH,
  PROP_PARAMS,
  PROP_MAX,
};

void eg_pipeline_asset_serialize(
    eg_pipeline_asset_t *pipeline_asset, eg_serializer_t *serializer) {
  eg_serializer_append_u32(serializer, PROP_MAX);

  // Vertex shader path
  eg_serializer_append_u32(serializer, PROP_VERT_PATH);
  eg_serializer_append_string(serializer, pipeline_asset->vert_path);

  // Fragment shader path
  eg_serializer_append_u32(serializer, PROP_FRAG_PATH);
  eg_serializer_append_string(serializer, pipeline_asset->frag_path);

  // Pipeline params
  eg_serializer_append_u32(serializer, PROP_PARAMS);
  eg_serializer_append(
      serializer, &pipeline_asset->params, sizeof(pipeline_asset->params));
}

void eg_pipeline_asset_deserialize(
    eg_pipeline_asset_t *pipeline_asset, eg_deserializer_t *deserializer) {
  uint32_t prop_count = eg_deserializer_read_u32(deserializer);

  eg_pipeline_asset_options_t options = {0};

  for (uint32_t i = 0; i < prop_count; i++) {
    uint32_t prop = eg_deserializer_read_u32(deserializer);

    switch (prop) {
    case PROP_VERT_PATH: {
      options.vert_path = eg_deserializer_read_string(deserializer);
      break;
    }
    case PROP_FRAG_PATH: {
      options.frag_path = eg_deserializer_read_string(deserializer);
      break;
    }
    case PROP_PARAMS: {
      eg_deserializer_read(
          deserializer, &options.params, sizeof(options.params));
      break;
    }
    default: break;
    }
  }

  eg_pipeline_asset_init(pipeline_asset, &options);
}
