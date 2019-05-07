#pragma once

#include "../assets/pipeline_asset.h"

typedef struct eg_renderable_comp_t {
  eg_pipeline_asset_t *pipeline;
} eg_renderable_comp_t;

void eg_renderable_comp_init(
    eg_renderable_comp_t *renderable, eg_pipeline_asset_t *pipeline);

void eg_renderable_comp_destroy(eg_renderable_comp_t *renderable);
