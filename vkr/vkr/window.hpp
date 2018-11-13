#pragma once

#include "aliases.hpp"
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

  void present(std::function<void()> drawFunction);

  // TODO: remove this function and automate its behaviour
  void updateSize();

  uint32_t getWidth() const;
  uint32_t getHeight() const;

  bool getRelativeMouse() const;
  void setRelativeMouse(bool relative = true);

  int getMouseX() const;
  int getMouseY() const;

  int getRelativeMouseX() const;
  int getRelativeMouseY() const;

  float getDelta() const;

  bool getShouldClose() const;
  void setShouldClose(bool shouldClose);

  SampleCount getMaxMSAASamples() const;
  SampleCount getMSAASamples() const;
  void setMSAASamples(SampleCount sampleCount);

  int getCurrentFrameIndex() const;
  CommandBuffer getCurrentCommandBuffer();

  void imguiBeginFrame();
  void imguiEndFrame();

  glm::vec4 clearColor{1.0f, 1.0f, 1.0f, 1.0f};

protected:
  bool shouldClose_ = false;

  static std::vector<const char *> requiredVulkanExtensions;

  SDL_Window *window_{nullptr};

  uint32_t lastTicks_ = 0;
  uint32_t deltaTicks_ = 0;

  vk::SurfaceKHR surface_;

  vk::SampleCountFlagBits msaaSamples_{vk::SampleCountFlagBits::e1};
  vk::SampleCountFlagBits maxMsaaSamples_{vk::SampleCountFlagBits::e1};

  vk::Format depthImageFormat_;

  struct {
    vk::Image image;
    VmaAllocation allocation;
    vk::ImageView view;
  } depthStencil_;

  struct FrameResources {
    vk::Semaphore imageAvailableSemaphore;
    vk::Semaphore renderingFinishedSemaphore;
    vk::Fence fence;

    vk::Framebuffer framebuffer;
    vk::Framebuffer imguiFramebuffer;

    vk::CommandBuffer commandBuffer;
  };

  // TODO: use fixed_vector
  std::vector<FrameResources> frameResources_{MAX_FRAMES_IN_FLIGHT};

  struct {
    struct {
      vk::Image image;
      VmaAllocation allocation;
      vk::ImageView view;
    } depth;

    struct {
      vk::Image image;
      VmaAllocation allocation;
      vk::ImageView view;
    } color;
  } multiSampleTargets_;

  // Current frame (capped by MAX_FRAMES_IN_FLIGHT)
  int currentFrame_ = 0;
  // Index of the current swapchain image
  uint32_t currentImageIndex_;

  vk::SwapchainKHR swapchain_;
  vk::Format swapchainImageFormat_;
  vk::Extent2D swapchainExtent_;
  std::vector<vk::Image> swapchainImages_;
  std::vector<vk::ImageView> swapchainImageViews_;

  vk::RenderPass renderPass_;
  vk::RenderPass imguiRenderPass_;

  void initVulkanExtensions() const;
  void createVulkanSurface();

  void createSyncObjects();
  void createSwapchain(uint32_t width, uint32_t height);
  void createSwapchainImageViews();

  void allocateGraphicsCommandBuffers();

  // Populates the depthStencil member struct
  void createDepthStencilResources();

  // Populates the multiSampleTargets struct
  void createMultisampleTargets();

  void createRenderPass();

  void createImguiRenderPass();

  void initImgui();

  void regenFramebuffer(
      vk::Framebuffer &framebuffer, vk::ImageView &swapchainImageView);

  void regenImguiFramebuffer(
      vk::Framebuffer &framebuffer, vk::ImageView &swapchainImageView);

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
