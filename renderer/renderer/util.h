#pragma once

#include <assert.h>
#include <stdio.h>
#include <vulkan/vulkan.h>

#define VK_CHECK(exp)                                                          \
  do {                                                                         \
    VkResult result = exp;                                                     \
    assert(result == VK_SUCCESS);                                              \
  } while (0)

#define _RE_LOG_INTERNAL(prefix, ...)                                          \
  do {                                                                         \
    printf(prefix __VA_ARGS__);                                                \
    puts("");                                                                  \
  } while (0);

#ifndef NDEBUG
#define RE_LOG_DEBUG(...) _RE_LOG_INTERNAL("[Renderer-debug] ", __VA_ARGS__);
#else
#define RE_LOG_DEBUG(...)
#endif

#define RE_LOG_INFO(...) _RE_LOG_INTERNAL("[Renderer-info] ", __VA_ARGS__);
#define RE_LOG_WARN(...) _RE_LOG_INTERNAL("[Renderer-warn] ", __VA_ARGS__);
#define RE_LOG_ERROR(...) _RE_LOG_INTERNAL("[Renderer-error] ", __VA_ARGS__);
#define RE_LOG_FATAL(...) _RE_LOG_INTERNAL("[Renderer-fatal] ", __VA_ARGS__);

void re_set_image_layout(
    VkCommandBuffer command_buffer,
    VkImage image,
    VkImageLayout old_image_layout,
    VkImageLayout new_image_layout,
    VkImageSubresourceRange subresource_range,
    VkPipelineStageFlags src_stage_mask,
    VkPipelineStageFlags dst_stage_mask);

static inline size_t re_format_pixel_size(VkFormat format) {
  switch (format) {
  case VK_FORMAT_R32_UINT:
    return sizeof(uint32_t);
  case VK_FORMAT_R8G8B8_UNORM:
    return sizeof(uint8_t) * 3;
  case VK_FORMAT_R8G8B8A8_UNORM:
    return sizeof(uint8_t) * 4;
  case VK_FORMAT_R32G32B32A32_SFLOAT:
    return sizeof(float) * 4;
  default:
    return 0;
  }
}
