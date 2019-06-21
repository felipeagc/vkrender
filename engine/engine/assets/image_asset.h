#pragma once

#include "asset_types.h"
#include <renderer/image.h>

typedef struct eg_asset_manager_t eg_asset_manager_t;
typedef struct eg_serializer_t eg_serializer_t;

typedef struct eg_image_asset_options_t {
  char *path;
} eg_image_asset_options_t;

typedef struct eg_image_asset_t {
  eg_asset_t asset;

  char *path;

  re_image_t image;
} eg_image_asset_t;

/*
 * Required asset functions
 */
eg_image_asset_t *eg_image_asset_create(
    eg_asset_manager_t *asset_manager, eg_image_asset_options_t *options);

void eg_image_asset_inspect(eg_image_asset_t *image, eg_inspector_t *inspector);

void eg_image_asset_destroy(eg_image_asset_t *image);

void eg_image_asset_serialize(
    eg_image_asset_t *image, eg_serializer_t *serializer);
