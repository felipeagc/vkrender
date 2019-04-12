#include "pipeline_asset.h"
#include "../pipelines.h"

void eg_pipeline_asset_init(
    eg_pipeline_asset_t *pipeline_asset,
    const re_render_target_t *render_target,
    const char *vertex_path,
    const char *fragment_path,
    const re_pipeline_parameters_t params) {
  assert(eg_init_pipeline_spv(
      &pipeline_asset->pipeline,
      render_target,
      vertex_path,
      fragment_path,
      params));
}

void eg_pipeline_asset_destroy(eg_pipeline_asset_t *pipeline_asset) {
  re_pipeline_destroy(&pipeline_asset->pipeline);
}
