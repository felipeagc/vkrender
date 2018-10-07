#pragma once

#include <SDL2/SDL.h>
#include <functional>
#include <string>
#include <vulkan/vk_mem_alloc.h>
#include <vulkan/vulkan.hpp>

namespace vkr {

class CommandBuffer;

const int MAX_FRAMES_IN_FLIGHT = 2;

class Window {
  friend class Context;
  friend class GraphicsPipeline;

public:
  Window(const char *title, uint32_t width = 800, uint32_t height = 600);
  ~Window();
  Window(const Window &other) = delete;
  Window &operator=(Window other) = delete;

  SDL_Event pollEvent();

  void present(std::function<void(CommandBuffer &)> drawFunction);
  // TODO: remove this function and automate its behaviour
  void updateSize();

  uint32_t getWidth() const;
  uint32_t getHeight() const;

  bool getShouldClose() const;
  void setShouldClose(bool shouldClose);

protected:
  bool shouldClose = false;

  static std::vector<const char *> requiredVulkanExtensions;

  SDL_Window *window{nullptr};

  vk::SurfaceKHR surface;

  vk::Format depthImageFormat;

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

  // Current frame (capped by MAX_FRAMES_IN_FLIGHT)
  int currentFrame = 0;
  // Index of the current swapchain image
  uint32_t currentImageIndex;

  vk::SwapchainKHR swapchain;
  vk::Format swapchainImageFormat;
  vk::Extent2D swapchainExtent;
  std::vector<vk::Image> swapchainImages;
  std::vector<vk::ImageView> swapchainImageViews;

  vk::RenderPass renderPass;

  void initVulkanExtensions() const;
  void createVulkanSurface();

  void createSyncObjects();
  void createSwapchain(uint32_t width, uint32_t height);
  void createSwapchainImageViews();

  void allocateGraphicsCommandBuffers();

  void createDepthResources();

  void createRenderPass();

  void regenFramebuffer(
      vk::Framebuffer &framebuffer,
      vk::ImageView colorImageView,
      vk::ImageView depthImageView);

  void destroyResizables();

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
