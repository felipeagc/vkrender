#pragma once

#include "asset_types.h"
#include <renderer/image.h>

typedef struct eg_image_asset_t {
  eg_asset_t asset;
  re_image_t image;
} eg_image_asset_t;

/*
 * Required asset functions
 */
void eg_image_asset_inspect(eg_image_asset_t *image, eg_inspector_t *inspector);

void eg_image_asset_destroy(eg_image_asset_t *image);

/*
 * Specific functions
 */
void eg_image_asset_init(eg_image_asset_t *image, const char *path);
