#include "world.hpp"

void eg_world_init(
    eg_world_t *world, eg_environment_asset_t *environment_asset) {
  eg_camera_init(&world->camera);
  eg_environment_init(&world->environment, environment_asset);
}

void eg_world_destroy(eg_world_t *world) {
  eg_environment_destroy(&world->environment);
  eg_camera_destroy(&world->camera);
}
