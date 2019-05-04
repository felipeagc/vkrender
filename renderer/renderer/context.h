#pragma once

#include "image.h"
#include <stdbool.h>
#include <tinycthread.h>
#include <vulkan/vk_mem_alloc.h>
#include <vulkan/vulkan.h>

typedef struct re_window_t re_window_t;

#ifndef NDEBUG
#define RE_ENABLE_VALIDATION
#endif

typedef struct re_context_t {
  VkInstance instance;
  VkDevice device;
  VkPhysicalDevice physical_device;

  VkDebugReportCallbackEXT debug_callback;

  uint32_t graphics_queue_family_index;
  uint32_t present_queue_family_index;
  uint32_t transfer_queue_family_index;

  mtx_t queue_mutex;
  VkQueue graphics_queue;
  VkQueue present_queue;
  VkQueue transfer_queue;

  VmaAllocator gpu_allocator;

  VkCommandPool graphics_command_pool;
  VkCommandPool transient_command_pool;

  VkDescriptorPool descriptor_pool;
  VkDescriptorSetLayout canvas_descriptor_set_layout;
} re_context_t;

extern re_context_t g_ctx;

void re_context_init();

VkSampleCountFlagBits re_context_get_max_sample_count();

bool re_context_get_supported_depth_format(VkFormat *depth_format);

void re_context_destroy();
