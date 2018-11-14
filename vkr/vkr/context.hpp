#pragma once

#include "descriptor_manager.hpp"
#include <functional>
#include <vector>
#include <vulkan/vk_mem_alloc.h>
#include <vulkan/vulkan.h>

namespace vkr {
namespace ctx {

#ifdef NDEBUG
const std::vector<const char *> REQUIRED_VALIDATION_LAYERS = {};
#else
const std::vector<const char *> REQUIRED_VALIDATION_LAYERS = {
    "VK_LAYER_LUNARG_standard_validation",
};
#endif

const std::vector<const char *> REQUIRED_DEVICE_EXTENSIONS = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
};

extern VkInstance instance;
extern VkDevice device;
extern VkPhysicalDevice physicalDevice;

extern VkDebugReportCallbackEXT callback;

extern uint32_t graphicsQueueFamilyIndex;
extern uint32_t presentQueueFamilyIndex;
extern uint32_t transferQueueFamilyIndex;

extern VkQueue graphicsQueue;
extern VkQueue presentQueue;
extern VkQueue transferQueue;

extern VmaAllocator allocator;

extern VkCommandPool graphicsCommandPool;
extern VkCommandPool transientCommandPool;

extern DescriptorManager descriptorManager;

void preInitialize(
    const std::vector<const char *> &requiredWindowVulkanExtensions);
void lateInitialize(VkSurfaceKHR &surface);
void destroy();

VkSampleCountFlagBits getMaxUsableSampleCount();

} // namespace ctx

} // namespace vkr
