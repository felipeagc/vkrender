#pragma once

#include "resource_manager.hpp"
#include "texture.hpp"
#include <util/thread.h>
#include <vulkan/vk_mem_alloc.h>
#include <vulkan/vulkan.h>

#ifndef NDEBUG
#define RE_ENABLE_VALIDATION
#endif

#define RE_THREAD_COUNT 4

#ifdef RE_ENABLE_VALIDATION
const char *const RE_REQUIRED_VALIDATION_LAYERS[] = {
    "VK_LAYER_LUNARG_standard_validation",
};
#else
const char *const RE_REQUIRED_VALIDATION_LAYERS[] = {};
#endif

const char *const RE_REQUIRED_DEVICE_EXTENSIONS[] = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
};

struct re_context_t {
  VkInstance instance;
  VkDevice device;
  VkPhysicalDevice physical_device;

  VkDebugReportCallbackEXT debug_callback;

  uint32_t graphics_queue_family_index;
  uint32_t present_queue_family_index;
  uint32_t transfer_queue_family_index;

  ut_mutex_t queue_mutex;
  VkQueue graphics_queue;
  VkQueue present_queue;
  VkQueue transfer_queue;

  VmaAllocator gpu_allocator;

  VkCommandPool graphics_command_pool;
  VkCommandPool transient_command_pool;

  VkCommandPool thread_command_pools[RE_THREAD_COUNT];

  re_resource_manager_t resource_manager;

  re_texture_t white_texture;
  re_texture_t black_texture;
};

extern re_context_t g_ctx;

void re_context_pre_init(
    re_context_t *ctx,
    const char *const *required_window_vulkan_extensions,
    uint32_t window_extension_count);

void re_context_late_inint(re_context_t *ctx, VkSurfaceKHR surface);

VkSampleCountFlagBits re_context_get_max_sample_count(re_context_t *ctx);

bool re_context_get_supported_depth_format(
    re_context_t *ctx, VkFormat *depthFormat);

void re_context_destroy(re_context_t *ctx);
