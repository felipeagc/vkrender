#pragma once

#include <renderer/image.h>

typedef struct eg_engine_t {
  re_image_t white_texture;
  re_image_t black_texture;
} eg_engine_t;

extern eg_engine_t g_eng;

void eg_engine_init();

void eg_engine_destroy();
