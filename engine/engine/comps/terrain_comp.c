#include "terrain_comp.h"

#include <renderer/context.h>
#include <stdlib.h>
#include <string.h>

void eg_terrain_comp_default(eg_terrain_comp_t *terrain) {
  memset(terrain, 0, sizeof(*terrain));
}

void eg_terrain_comp_inspect(
    eg_terrain_comp_t *terrain, eg_inspector_t *inspector) {}

void eg_terrain_comp_destroy(eg_terrain_comp_t *terrain) {}
