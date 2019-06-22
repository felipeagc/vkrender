#pragma once

#include "hasher.h"
#include "vulkan.h"

typedef struct re_render_target_t {
  VkSampleCountFlags sample_count;
  VkRenderPass render_pass;
  uint32_t width;
  uint32_t height;
  re_hash_t hash;
} re_render_target_t;

re_hash_t re_hash_renderpass(VkRenderPassCreateInfo *create_info);
