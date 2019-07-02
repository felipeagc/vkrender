#include "scene.h"

#include "asset_manager.h"
#include "assets/image_asset.h"
#include "deserializer.h"
#include "serializer.h"

void eg_scene_init(
    eg_scene_t *scene,
    eg_image_asset_t *skybox,
    eg_image_asset_t *irradiance,
    eg_image_asset_t *radiance,
    eg_image_asset_t *brdf) {
  eg_camera_init(&scene->camera);
  eg_environment_init(&scene->environment, skybox, irradiance, radiance, brdf);

  eg_entity_manager_init(&scene->entity_manager);
}

void eg_scene_destroy(eg_scene_t *scene) {
  eg_environment_destroy(&scene->environment);
  eg_camera_destroy(&scene->camera);

  eg_entity_manager_destroy(&scene->entity_manager);
}

enum {
  PROP_ENV_SKYBOX,
  PROP_ENV_IRRADIANCE,
  PROP_ENV_RADIANCE,
  PROP_ENV_BRDF,
  PROP_ENV_UNIFORM,
  PROP_MAX,
};

void eg_scene_serialize(eg_scene_t *scene, eg_serializer_t *serializer) {
  eg_serializer_append_u32(serializer, PROP_MAX);

  eg_serializer_append_u32(serializer, PROP_ENV_SKYBOX);
  eg_serializer_append_u32(serializer, scene->environment.skybox->asset.uid);

  eg_serializer_append_u32(serializer, PROP_ENV_IRRADIANCE);
  eg_serializer_append_u32(
      serializer, scene->environment.irradiance->asset.uid);

  eg_serializer_append_u32(serializer, PROP_ENV_RADIANCE);
  eg_serializer_append_u32(serializer, scene->environment.radiance->asset.uid);

  eg_serializer_append_u32(serializer, PROP_ENV_BRDF);
  eg_serializer_append_u32(serializer, scene->environment.brdf->asset.uid);

  eg_serializer_append_u32(serializer, PROP_ENV_UNIFORM);
  eg_serializer_append(
      serializer,
      &scene->environment.uniform,
      sizeof(scene->environment.uniform));
}

void eg_scene_deserialize(eg_scene_t *scene, eg_deserializer_t *deserializer) {
  uint32_t prop_count = eg_deserializer_read_u32(deserializer);

  eg_image_asset_t *skybox     = NULL;
  eg_image_asset_t *irradiance = NULL;
  eg_image_asset_t *radiance   = NULL;
  eg_image_asset_t *brdf       = NULL;
  eg_environment_uniform_t env_uniform;

  for (uint32_t i = 0; i < prop_count; i++) {
    uint32_t prop = eg_deserializer_read_u32(deserializer);

    switch (prop) {
    case PROP_ENV_SKYBOX: {
      uint32_t uid = eg_deserializer_read_u32(deserializer);
      skybox = eg_asset_manager_get_by_uid(deserializer->asset_manager, uid);
      break;
    }
    case PROP_ENV_IRRADIANCE: {
      uint32_t uid = eg_deserializer_read_u32(deserializer);
      irradiance =
          eg_asset_manager_get_by_uid(deserializer->asset_manager, uid);
      break;
    }
    case PROP_ENV_RADIANCE: {
      uint32_t uid = eg_deserializer_read_u32(deserializer);
      radiance = eg_asset_manager_get_by_uid(deserializer->asset_manager, uid);
      break;
      break;
    }
    case PROP_ENV_BRDF: {
      uint32_t uid = eg_deserializer_read_u32(deserializer);
      brdf = eg_asset_manager_get_by_uid(deserializer->asset_manager, uid);
      break;
    }
    case PROP_ENV_UNIFORM: {
      eg_deserializer_read(deserializer, &env_uniform, sizeof(env_uniform));
      break;
    }
    default: break;
    }
  }

  eg_camera_init(&scene->camera);
  eg_environment_init(&scene->environment, skybox, irradiance, radiance, brdf);
  scene->environment.uniform = env_uniform;
}
