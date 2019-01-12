#pragma once

#include <vulkan/vulkan.h>

struct re_render_target_t {
  VkSampleCountFlagBits sample_count;
  VkRenderPass render_pass;
};
