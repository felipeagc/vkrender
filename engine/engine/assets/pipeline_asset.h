#pragma once

#include "asset_types.h"
#include <renderer/pipeline.h>

typedef struct eg_pipeline_asset_t {
  eg_asset_t asset;

  re_pipeline_t pipeline;
} eg_pipeline_asset_t;

void eg_pipeline_asset_init(
    eg_pipeline_asset_t *pipeline_asset,
    const re_render_target_t *render_target,
    const char *paths[],
    uint32_t path_count,
    const re_pipeline_parameters_t params);

void eg_pipeline_asset_destroy(eg_pipeline_asset_t *pipeline_asset);
