#include "pipeline_asset.h"

#include "../imgui.h"
#include "../pipelines.h"

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

void eg_pipeline_asset_init(
    eg_pipeline_asset_t *pipeline_asset,
    const re_render_target_t *render_target,
    const char *paths[],
    uint32_t path_count,
    const re_pipeline_parameters_t params) {
  eg_init_pipeline_spv(
      &pipeline_asset->pipeline, render_target, paths, path_count, params);
}
