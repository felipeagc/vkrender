#include "scene.h"

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
