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

#define RE_THREAD_COUNT 4

#ifdef RE_ENABLE_VALIDATION
static const char *const RE_REQUIRED_VALIDATION_LAYERS[] = {
    "VK_LAYER_LUNARG_standard_validation",
};
#else
static const char *const RE_REQUIRED_VALIDATION_LAYERS[] = {};
#endif

static const char *const RE_REQUIRED_DEVICE_EXTENSIONS[] = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
};

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

  VkCommandPool thread_command_pools[RE_THREAD_COUNT];

  VkDescriptorPool descriptor_pool;
  VkDescriptorSetLayout canvas_descriptor_set_layout;

  re_image_t white_texture;
  re_image_t black_texture;
} re_context_t;

extern re_context_t g_ctx;

void re_context_init(re_window_t *window);

void re_context_init_graphics(re_window_t *window);

VkSampleCountFlagBits re_context_get_max_sample_count();

bool re_context_get_supported_depth_format(VkFormat *depth_format);

void re_context_destroy();
