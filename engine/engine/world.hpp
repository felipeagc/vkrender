#pragma once

#include "camera.hpp"
#include "environment.hpp"

typedef struct eg_world_t {
  eg_camera_t camera;
  eg_environment_t environment;
} eg_world_t;

void eg_world_init(
    eg_world_t *world, eg_environment_asset_t *environment_asset);

void eg_world_destroy(eg_world_t *world);
