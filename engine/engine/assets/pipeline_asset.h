#pragma once

#include "asset_types.h"
#include <renderer/pipeline.h>

typedef struct eg_inspector_t eg_inspector_t;

typedef struct eg_pipeline_asset_t {
  eg_asset_t asset;

  re_pipeline_t pipeline;
} eg_pipeline_asset_t;

/*
 * Required asset functions
 */
void eg_pipeline_asset_inspect(
    eg_pipeline_asset_t *pipeline_asset, eg_inspector_t *inspector);

void eg_pipeline_asset_destroy(eg_pipeline_asset_t *pipeline_asset);

/*
 * Specific functions
 */
void eg_pipeline_asset_init(
    eg_pipeline_asset_t *pipeline_asset,
    const re_render_target_t *render_target,
    const char *paths[],
    uint32_t path_count,
    const re_pipeline_parameters_t params);
