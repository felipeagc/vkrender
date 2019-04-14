#pragma once

#include <vulkan/vulkan.h>

typedef struct re_render_target_t {
  VkSampleCountFlagBits sample_count;
  VkRenderPass render_pass;
  uint32_t width;
  uint32_t height;
} re_render_target_t;
