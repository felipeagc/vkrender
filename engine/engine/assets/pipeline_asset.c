#include "pipeline_asset.h"

#include "../asset_manager.h"
#include "../imgui.h"
#include "../pipelines.h"

eg_pipeline_asset_t *eg_pipeline_asset_create(
    eg_asset_manager_t *asset_manager, eg_pipeline_asset_options_t *options) {
  eg_pipeline_asset_t *pipeline_asset =
      eg_asset_manager_alloc(asset_manager, EG_ASSET_TYPE(eg_pipeline_asset_t));

  eg_init_pipeline_spv(
      &pipeline_asset->pipeline,
      options->render_target,
      (const char *[]){options->vert, options->frag},
      2,
      options->params);

  return pipeline_asset;
}

void eg_pipeline_asset_inspect(
    eg_pipeline_asset_t *pipeline_asset, eg_inspector_t *inspector) {
  igText("Pipeline: %#010x", (uint64_t)pipeline_asset->pipeline.pipeline);

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
}
