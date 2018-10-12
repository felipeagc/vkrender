#pragma once

#include "descriptor_manager.hpp"
#include "util.hpp"
#include <functional>
#include <vector>
#include <vulkan/vk_mem_alloc.h>
#include <vulkan/vulkan.hpp>

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

  static Device &getDevice() {
    return Context::get().device;
  }

  static DescriptorManager &getDescriptorManager() {
    return Context::get().descriptorManager;
  }

protected:
  vk::Instance instance;

  vk::DebugReportCallbackEXT callback;

  vk::PhysicalDevice physicalDevice;
  vk::Device device;

  uint32_t graphicsQueueFamilyIndex = UINT32_MAX;
  uint32_t presentQueueFamilyIndex = UINT32_MAX;
  uint32_t transferQueueFamilyIndex = UINT32_MAX;

  vk::Queue graphicsQueue;
  vk::Queue presentQueue;
  vk::Queue transferQueue;

  VmaAllocator allocator;

  vk::CommandPool graphicsCommandPool;
  vk::CommandPool transientCommandPool;

  vk::Format depthImageFormat;

  DescriptorManager descriptorManager;

  void createInstance();

  void lazyInit(vk::SurfaceKHR &surface);

  void createDevice(vk::SurfaceKHR &surface);
  void getDeviceQueues();

  void setupMemoryAllocator();

  void createGraphicsCommandPool();
  void createTransientCommandPool();

  std::vector<const char *>
  getRequiredExtensions(std::vector<const char *> sdlExtensions);
  bool checkValidationLayerSupport();
  void setupDebugCallback();
  bool checkPhysicalDeviceProperties(
      vk::PhysicalDevice physicalDevice,
      vk::SurfaceKHR &surface,
      uint32_t *graphicsQueue,
      uint32_t *presentQueue,
      uint32_t *transferQueue);
};

} // namespace vkr
