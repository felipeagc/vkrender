#pragma once

#include "../pipelines.h"
#include "asset_types.h"
#include <renderer/pipeline.h>

typedef struct eg_inspector_t eg_inspector_t;

typedef struct eg_pipeline_asset_options_t {
  char *vert_path;
  char *frag_path;
  eg_pipeline_params_t params;
} eg_pipeline_asset_options_t;

typedef struct eg_pipeline_asset_t {
  eg_asset_t asset;

  re_pipeline_t pipeline;

  char *vert_path;
  char *frag_path;
  eg_pipeline_params_t params;
} eg_pipeline_asset_t;

/*
 * Required asset functions
 */
void eg_pipeline_asset_init(
    eg_pipeline_asset_t *pipeline_asset, eg_pipeline_asset_options_t *options);

void eg_pipeline_asset_inspect(
    eg_pipeline_asset_t *pipeline_asset, eg_inspector_t *inspector);

void eg_pipeline_asset_destroy(eg_pipeline_asset_t *pipeline_asset);

void eg_pipeline_asset_serialize(
    eg_pipeline_asset_t *pipeline_asset, eg_serializer_t *serializer);

void eg_pipeline_asset_deserialize(
    eg_pipeline_asset_t *pipeline_asset, eg_deserializer_t *deserializer);
