#pragma once

#include "buffer_pool.h"
#include "image.h"
#include "vulkan.h"
#include <stdbool.h>
#include <tinycthread.h>
#include <vma/vk_mem_alloc.h>

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

  VkPhysicalDeviceLimits physical_limits;

  // TODO: we'll have to GC these (maybe store them in a hash table)
  uint32_t descriptor_set_allocator_count;
  re_descriptor_set_allocator_t *descriptor_set_allocators;

  re_buffer_pool_t ubo_pool;
} re_context_t;

extern re_context_t g_ctx;

void re_ctx_init();

void re_ctx_destroy();

void re_ctx_begin_frame();

re_descriptor_set_allocator_t *
rx_ctx_request_descriptor_set_allocator(re_descriptor_set_layout_t layout);

// TODO: new function
// void re_ctx_gc();

VkSampleCountFlagBits re_ctx_get_max_sample_count();

bool re_ctx_get_supported_depth_format(VkFormat *depth_format);
