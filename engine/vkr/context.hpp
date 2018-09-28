#pragma once

#include <vector>
#include <vulkan/vk_mem_alloc.h>
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

const int MAX_FRAMES_IN_FLIGHT = 2;

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

  struct FrameResources {
    vk::Image depthImage;
    VmaAllocation depthImageAllocation;
    vk::ImageView depthImageView;

    vk::Semaphore imageAvailableSemaphore;
    vk::Semaphore renderingFinishedSemaphore;
    vk::Fence fence;

    vk::Framebuffer framebuffer;

    vk::CommandBuffer commandBuffer;
  };

  std::vector<FrameResources> frameResources{MAX_FRAMES_IN_FLIGHT};

  vk::SwapchainKHR swapchain;
  vk::Format swapchainImageFormat;
  vk::Extent2D swapchainExtent;
  std::vector<vk::Image> swapchainImages;
  std::vector<vk::ImageView> swapchainImageViews;

  vk::CommandPool graphicsCommandPool;
  vk::CommandPool transientCommandPool;

  void createInstance(std::vector<const char *> sdlExtensions);
  void createDevice();
  void getDeviceQueues();

  void createSyncObjects();

  void createSwapchain(uint32_t width, uint32_t height);
  void createSwapchainImageViews();

  void createGraphicsCommandPool();
  void createTransientCommandPool();

  void allocateGraphicsCommandBuffers();


  std::vector<const char *>
  getRequiredExtensions(std::vector<const char *> sdlExtensions);
  bool checkValidationLayerSupport();
  void setupDebugCallback();
  bool checkPhysicalDeviceProperties(
      vk::PhysicalDevice physicalDevice,
      uint32_t *graphicsQueue,
      uint32_t *presentQueue,
      uint32_t *transferQueue);

  uint32_t
  getSwapchainNumImages(const vk::SurfaceCapabilitiesKHR &surfaceCapabilities);
  vk::SurfaceFormatKHR
  getSwapchainFormat(const std::vector<vk::SurfaceFormatKHR> &formats);
  vk::Extent2D getSwapchainExtent(
      uint32_t width,
      uint32_t height,
      const vk::SurfaceCapabilitiesKHR &surfaceCapabilities);
  vk::ImageUsageFlags
  getSwapchainUsageFlags(const vk::SurfaceCapabilitiesKHR &surfaceCapabilities);
  vk::SurfaceTransformFlagBitsKHR
  getSwapchainTransform(const vk::SurfaceCapabilitiesKHR &surfaceCapabilities);
  vk::PresentModeKHR
  getSwapchainPresentMode(const std::vector<vk::PresentModeKHR> &presentModes);
};
} // namespace vkr
