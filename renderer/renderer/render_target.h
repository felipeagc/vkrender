#pragma once

#include "vulkan.h"

typedef struct re_render_target_t {
  VkSampleCountFlags sample_count;
  VkRenderPass render_pass;
  uint32_t width;
  uint32_t height;
} re_render_target_t;
