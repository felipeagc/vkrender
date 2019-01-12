#pragma once

#include "common.hpp"
#include "glm.hpp"
#include "render_target.hpp"
#include "scancodes.hpp"
#include <SDL2/SDL.h>
#include <chrono>
#include <ftl/vector.hpp>
#include <functional>
#include <string>
#include <vulkan/vk_mem_alloc.h>
#include <vulkan/vulkan.h>

namespace renderer {

class Window {
  friend class ImGuiRenderer;

public:
  Window(const char *title, uint32_t width = 800, uint32_t height = 600);
  ~Window();

  // No copying Window
  Window(const Window &) = delete;
  Window &operator=(const Window &) = delete;

  bool pollEvent(SDL_Event *event);

  void beginFrame();
  void endFrame();

  void beginRenderPass();
  void endRenderPass();

  uint32_t getWidth() const;
  uint32_t getHeight() const;

  bool getRelativeMouse() const;
  void setRelativeMouse(bool relative = true);

  void getMouseState(int *x, int *y) const;
  void getRelativeMouseState(int *x, int *y) const;

  void warpMouse(int x, int y);

  bool isMouseLeftPressed() const;
  bool isMouseRightPressed() const;

  bool isScancodePressed(Scancode scancode) const;

  double getDelta() const;

  bool getShouldClose() const;
  void setShouldClose(bool shouldClose);

  VkSampleCountFlagBits getMaxSampleCount() const;

  int getCurrentFrameIndex() const;
  VkCommandBuffer getCurrentCommandBuffer();

  glm::vec4 clearColor{1.0f, 1.0f, 1.0f, 1.0f};

  re_render_target_t render_target;

protected:
  bool m_shouldClose = false;

  SDL_Window *m_window = nullptr;

  double m_deltaTime = 0.0f;
  std::chrono::time_point<std::chrono::high_resolution_clock> m_timeBefore;

  VkSurfaceKHR m_surface = VK_NULL_HANDLE;

  VkSampleCountFlagBits m_maxSampleCount = VK_SAMPLE_COUNT_1_BIT;

  VkFormat m_depthImageFormat;

  // @note: might wanna make one of these per frame
  struct {
    VkImage image = VK_NULL_HANDLE;
    VmaAllocation allocation = VK_NULL_HANDLE;
    VkImageView view = VK_NULL_HANDLE;
  } m_depthStencil;

  struct FrameResources {
    VkSemaphore imageAvailableSemaphore = VK_NULL_HANDLE;
    VkSemaphore renderingFinishedSemaphore = VK_NULL_HANDLE;
    VkFence fence = VK_NULL_HANDLE;

    VkFramebuffer framebuffer = VK_NULL_HANDLE;

    VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
  };

  ftl::small_vector<FrameResources> m_frameResources{MAX_FRAMES_IN_FLIGHT};

  // Current frame (capped by MAX_FRAMES_IN_FLIGHT)
  int m_currentFrame = 0;
  // Index of the current swapchain image
  uint32_t m_currentImageIndex;

  VkSwapchainKHR m_swapchain = VK_NULL_HANDLE;
  VkFormat m_swapchainImageFormat;
  VkExtent2D m_swapchainExtent;
  ftl::small_vector<VkImage> m_swapchainImages;
  ftl::small_vector<VkImageView> m_swapchainImageViews;

  void createVulkanSurface();

  void createSyncObjects();
  void createSwapchain(uint32_t width, uint32_t height);
  void createSwapchainImageViews();

  void allocateGraphicsCommandBuffers();

  // Populates the depthStencil member struct
  void createDepthStencilResources();

  void createRenderPass();

  void
  regenFramebuffer(VkFramebuffer &framebuffer, VkImageView &swapchainImageView);

  // When window gets resized, call this.
  void updateSize();

  void destroyResizables();

  uint32_t
  getSwapchainNumImages(const VkSurfaceCapabilitiesKHR &surfaceCapabilities);
  VkSurfaceFormatKHR
  getSwapchainFormat(const ftl::small_vector<VkSurfaceFormatKHR> &formats);
  VkExtent2D getSwapchainExtent(
      uint32_t width,
      uint32_t height,
      const VkSurfaceCapabilitiesKHR &surfaceCapabilities);
  VkImageUsageFlags
  getSwapchainUsageFlags(const VkSurfaceCapabilitiesKHR &surfaceCapabilities);
  VkSurfaceTransformFlagBitsKHR
  getSwapchainTransform(const VkSurfaceCapabilitiesKHR &surfaceCapabilities);
  VkPresentModeKHR getSwapchainPresentMode(
      const ftl::small_vector<VkPresentModeKHR> &presentModes);
};
} // namespace renderer
