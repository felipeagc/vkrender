#pragma once

#include <stdlib.h>

typedef struct eg_inspector_t eg_inspector_t;
typedef struct eg_pipeline_asset_t eg_pipeline_asset_t;
typedef struct re_pipeline_t re_pipeline_t;

typedef struct eg_renderable_comp_t {
  eg_pipeline_asset_t *pipeline;
} eg_renderable_comp_t;

/*
 * Required component functions
 */
void eg_renderable_comp_default(eg_renderable_comp_t *renderable);

void eg_renderable_comp_inspect(
    eg_renderable_comp_t *renderable, eg_inspector_t *inspector);

void eg_renderable_comp_destroy(eg_renderable_comp_t *renderable);

/*
 * Specific functions
 */
void eg_renderable_comp_init(
    eg_renderable_comp_t *renderable, eg_pipeline_asset_t *pipeline);

re_pipeline_t *
eg_renderable_comp_get_pipeline(eg_renderable_comp_t *renderable);
