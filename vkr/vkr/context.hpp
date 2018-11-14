#pragma once

#include "descriptor_manager.hpp"
#include <functional>
#include <vector>
#include <vulkan/vk_mem_alloc.h>
#include <vulkan/vulkan.h>

namespace vkr {
class Window;
class CommandBuffer;

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

class Context {
  friend class Window;
  friend class Buffer;
  friend class StagingBuffer;
  friend class Texture;

public:
  Context();
  ~Context();
  Context(Context const &) = delete;
  Context &operator=(Context const &) = delete;

  static Context &get() {
    static Context context;
    return context;
  }

  static VkDevice &getDevice() { return Context::get().device_; }

  static VkPhysicalDevice &getPhysicalDevice() {
    return Context::get().physicalDevice_;
  }

  static DescriptorManager &getDescriptorManager() {
    return Context::get().descriptorManager_;
  }

protected:
  VkInstance instance_{VK_NULL_HANDLE};

  VkDebugReportCallbackEXT callback_{VK_NULL_HANDLE};

  VkPhysicalDevice physicalDevice_{VK_NULL_HANDLE};
  VkDevice device_{VK_NULL_HANDLE};

  uint32_t graphicsQueueFamilyIndex_ = UINT32_MAX;
  uint32_t presentQueueFamilyIndex_ = UINT32_MAX;
  uint32_t transferQueueFamilyIndex_ = UINT32_MAX;

  VkQueue graphicsQueue_{VK_NULL_HANDLE};
  VkQueue presentQueue_{VK_NULL_HANDLE};
  VkQueue transferQueue_{VK_NULL_HANDLE};

  VmaAllocator allocator_{VK_NULL_HANDLE};

  VkCommandPool graphicsCommandPool_{VK_NULL_HANDLE};
  VkCommandPool transientCommandPool_{VK_NULL_HANDLE};

  DescriptorManager descriptorManager_;

  void createInstance();

  void lazyInit(VkSurfaceKHR &surface);

  void createDevice(VkSurfaceKHR &surface);
  void getDeviceQueues();

  void setupMemoryAllocator();

  void createGraphicsCommandPool();
  void createTransientCommandPool();

  VkSampleCountFlagBits getMaxUsableSampleCount();

  std::vector<const char *>
  getRequiredExtensions(std::vector<const char *> sdlExtensions);

  bool checkValidationLayerSupport();

  void setupDebugCallback();

  bool checkPhysicalDeviceProperties(
      VkPhysicalDevice physicalDevice,
      VkSurfaceKHR &surface,
      uint32_t *graphicsQueue,
      uint32_t *presentQueue,
      uint32_t *transferQueue);
};

} // namespace vkr
