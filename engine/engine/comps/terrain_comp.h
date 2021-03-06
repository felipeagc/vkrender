#pragma once

#include <renderer/image.h>

typedef struct eg_inspector_t eg_inspector_t;
typedef struct eg_serializer_t eg_serializer_t;
typedef struct eg_deserializer_t eg_deserializer_t;

typedef struct eg_terrain_comp_t {
  uint32_t temp;
} eg_terrain_comp_t;

/*
 * Required component functions
 */
void eg_terrain_comp_default(eg_terrain_comp_t *terrain);

void eg_terrain_comp_inspect(
    eg_terrain_comp_t *terrain, eg_inspector_t *inspector);

void eg_terrain_comp_destroy(eg_terrain_comp_t *terrain);

void eg_terrain_comp_serialize(
    eg_terrain_comp_t *terrain, eg_serializer_t *serializer);

void eg_terrain_comp_deserialize(
    eg_terrain_comp_t *terrain, eg_deserializer_t *deserializer);
