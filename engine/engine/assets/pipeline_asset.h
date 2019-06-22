#pragma once

#include "asset_types.h"
#include "../pipelines.h"
#include <renderer/pipeline.h>

typedef struct eg_inspector_t eg_inspector_t;
typedef struct eg_serializer_t eg_serializer_t;

// TODO: make params a simpler struct, only with important options
typedef struct eg_pipeline_asset_options_t {
  const char *vert_path;
  const char *frag_path;
  const eg_pipeline_params_t params;
} eg_pipeline_asset_options_t;

typedef struct eg_pipeline_asset_t {
  eg_asset_t asset;

  re_pipeline_t pipeline;

  char *vert_path;
  char *frag_path;
} eg_pipeline_asset_t;

/*
 * Required asset functions
 */
eg_pipeline_asset_t *eg_pipeline_asset_create(
    eg_asset_manager_t *asset_manager, eg_pipeline_asset_options_t *options);

void eg_pipeline_asset_inspect(
    eg_pipeline_asset_t *pipeline_asset, eg_inspector_t *inspector);

void eg_pipeline_asset_destroy(eg_pipeline_asset_t *pipeline_asset);

void eg_pipeline_asset_serialize(
    eg_pipeline_asset_t *pipeline_asset, eg_serializer_t *serializer);
