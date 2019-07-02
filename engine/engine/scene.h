#pragma once

#include "camera.h"
#include "entity_manager.h"
#include "environment.h"

typedef struct eg_serializer_t eg_serializer_t;
typedef struct eg_deserializer_t eg_deserializer_t;

typedef struct eg_scene_t {
  eg_camera_t camera;
  eg_environment_t environment;

  eg_entity_manager_t entity_manager;
} eg_scene_t;

void eg_scene_init(
    eg_scene_t *scene,
    eg_image_asset_t *skybox,
    eg_image_asset_t *irradiance,
    eg_image_asset_t *radiance,
    eg_image_asset_t *brdf);

void eg_scene_destroy(eg_scene_t *scene);

void eg_scene_serialize(eg_scene_t *scene, eg_serializer_t *serializer);

void eg_scene_deserialize(eg_scene_t *scene, eg_deserializer_t *deserializer);
