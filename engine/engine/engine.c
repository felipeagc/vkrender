#include "engine.h"

#include <renderer/context.h>

eg_engine_t g_eng;

void eg_engine_init() {
  re_image_init(
      &g_eng.white_texture,
      g_ctx.transient_command_pool,
      &(re_image_options_t){
          .data = (uint8_t[]){255, 255, 255, 255},
          .width = 1,
          .height = 1,
          .format = VK_FORMAT_R8G8B8A8_UNORM,
      });

  re_image_init(
      &g_eng.black_texture,
      g_ctx.transient_command_pool,
      &(re_image_options_t){
          .data = (uint8_t[]){0, 0, 0, 255},
          .width = 1,
          .height = 1,
          .format = VK_FORMAT_R8G8B8A8_UNORM,
      });
}

void eg_engine_destroy() {
  re_image_destroy(&g_eng.white_texture);
  re_image_destroy(&g_eng.black_texture);
}
