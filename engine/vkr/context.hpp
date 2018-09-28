#pragma once

#include <vector>
#include <vulkan/vulkan.hpp>

namespace vkr {
class Window;

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
public:
  Context(const Window &window);
  ~Context();
  Context(const Context &other) = delete;
  Context &operator=(Context other) = delete;

private:
  vk::Instance instance;
  vk::SurfaceKHR surface;

  vk::DebugReportCallbackEXT callback;

  vk::PhysicalDevice physicalDevice;
  vk::Device device;

  uint32_t graphicsQueueFamilyIndex = UINT32_MAX;
  uint32_t presentQueueFamilyIndex = UINT32_MAX;
  uint32_t transferQueueFamilyIndex = UINT32_MAX;

  vk::Queue graphicsQueue;
  vk::Queue presentQueue;
  vk::Queue transferQueue;

  void createInstance(std::vector<const char *> sdlExtensions);
  void createDevice();
  void getDeviceQueues();

  std::vector<const char *>
  getRequiredExtensions(std::vector<const char *> sdlExtensions);
  bool checkValidationLayerSupport();
  void setupDebugCallback();
  bool checkPhysicalDeviceProperties(
      vk::PhysicalDevice physicalDevice,
      uint32_t *graphicsQueue,
      uint32_t *presentQueue,
      uint32_t *transferQueue);
};
} // namespace vkr
