#pragma once

#include "resource_manager.hpp"
#include "texture.hpp"
#include <mutex>
#include <vector>
#include <vulkan/vk_mem_alloc.h>
#include <vulkan/vulkan.h>

#ifndef NDEBUG
#define VKR_ENABLE_VALIDATION
#endif

namespace renderer {

const uint32_t VKR_THREAD_COUNT = 4;

class Context;

Context &ctx();

#ifdef VKR_ENABLE_VALIDATION
const std::vector<const char *> REQUIRED_VALIDATION_LAYERS = {
    "VK_LAYER_LUNARG_standard_validation",
};
#else
const std::vector<const char *> REQUIRED_VALIDATION_LAYERS = {};
#endif

const std::vector<const char *> REQUIRED_DEVICE_EXTENSIONS = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
};

class Context {
public:
  Context();
  ~Context();

  Context(const Context &) = delete;
  Context &operator=(const Context &) = delete;

  void preInitialize(
      const std::vector<const char *> &requiredWindowVulkanExtensions);
  void lateInitialize(VkSurfaceKHR &surface);

  VkSampleCountFlagBits getMaxUsableSampleCount();

  bool getSupportedDepthFormat(VkFormat *depthFormat);

  VkInstance m_instance = VK_NULL_HANDLE;
  VkDevice m_device = VK_NULL_HANDLE;
  VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;

  VkDebugReportCallbackEXT m_callback = VK_NULL_HANDLE;

  uint32_t m_graphicsQueueFamilyIndex = -1;
  uint32_t m_presentQueueFamilyIndex = -1;
  uint32_t m_transferQueueFamilyIndex = -1;

  std::mutex m_queueMutex;
  VkQueue m_graphicsQueue = VK_NULL_HANDLE;
  VkQueue m_presentQueue = VK_NULL_HANDLE;
  VkQueue m_transferQueue = VK_NULL_HANDLE;

  VmaAllocator m_allocator = VK_NULL_HANDLE;

  VkCommandPool m_graphicsCommandPool = VK_NULL_HANDLE;
  VkCommandPool m_transientCommandPool = VK_NULL_HANDLE;

  VkCommandPool m_threadCommandPools[VKR_THREAD_COUNT];

  re_resource_manager_t resource_manager;

  re_texture_t m_white_texture;
  re_texture_t m_black_texture;

private:
  void createInstance(
      const std::vector<const char *> &requiredWindowVulkanExtensions);
  void setupDebugCallback();
  void createDevice(VkSurfaceKHR &surface);
  void getDeviceQueues();
  void setupMemoryAllocator();
  void createGraphicsCommandPool();
  void createTransientCommandPool();
  void createThreadCommandPools();
};

} // namespace renderer
