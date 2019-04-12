#pragma once

#include <renderer/cmd_buffer.h>

typedef struct eg_cmd_info_t {
  re_cmd_buffer_t cmd_buffer;
  uint32_t frame_index;
} eg_cmd_info_t;
