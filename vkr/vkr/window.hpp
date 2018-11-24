#pragma once

#include <SDL2/SDL.h>
#include <fstl/fixed_vector.hpp>
#include <functional>
#include <string>
#include <vulkan/vk_mem_alloc.h>
#include <vulkan/vulkan.h>

namespace vkr {

const int MAX_FRAMES_IN_FLIGHT = 2;

class Window {
public:
  Window(
      const char *title,
      uint32_t width = 800,
      uint32_t height = 600,
      VkSampleCountFlagBits sampleCount = VK_SAMPLE_COUNT_1_BIT);
  ~Window(){};
  Window(const Window &other) = delete;
  Window &operator=(Window other) = delete;

  // TODO: replace with destructor
  void destroy();

  bool pollEvent(SDL_Event *event);

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

  bool isMouseLeftPressed() const;
  bool isMouseRightPressed() const;

  double getDelta() const;

  bool getShouldClose() const;
  void setShouldClose(bool shouldClose);

  VkSampleCountFlagBits getMaxMSAASamples() const;
  VkSampleCountFlagBits getMSAASamples() const;

  int getCurrentFrameIndex() const;
  VkCommandBuffer getCurrentCommandBuffer();

  void imguiBeginFrame();
  void imguiEndFrame();

  VkRenderPass getRenderPass();

  glm::vec4 clearColor{1.0f, 1.0f, 1.0f, 1.0f};

protected:
  bool m_shouldClose = false;

  SDL_Window *m_window{nullptr};

  double m_deltaTime = 0.0f;

  VkSurfaceKHR m_surface{VK_NULL_HANDLE};

  VkSampleCountFlagBits m_msaaSamples{VK_SAMPLE_COUNT_1_BIT};
  VkSampleCountFlagBits m_maxMsaaSamples{VK_SAMPLE_COUNT_1_BIT};

  VkFormat m_depthImageFormat;

  struct {
    VkImage image{VK_NULL_HANDLE};
    VmaAllocation allocation{VK_NULL_HANDLE};
    VkImageView view{VK_NULL_HANDLE};
  } m_depthStencil;

  struct FrameResources {
    VkSemaphore imageAvailableSemaphore{VK_NULL_HANDLE};
    VkSemaphore renderingFinishedSemaphore{VK_NULL_HANDLE};
    VkFence fence{VK_NULL_HANDLE};

    VkFramebuffer framebuffer{VK_NULL_HANDLE};
    VkFramebuffer imguiFramebuffer{VK_NULL_HANDLE};

    VkCommandBuffer commandBuffer{VK_NULL_HANDLE};
  };

  fstl::fixed_vector<FrameResources> m_frameResources{MAX_FRAMES_IN_FLIGHT};

  struct {
    struct {
      VkImage image{VK_NULL_HANDLE};
      VmaAllocation allocation{VK_NULL_HANDLE};
      VkImageView view{VK_NULL_HANDLE};
    } depth;

    struct {
      VkImage image{VK_NULL_HANDLE};
      VmaAllocation allocation{VK_NULL_HANDLE};
      VkImageView view{VK_NULL_HANDLE};
    } color;
  } m_multiSampleTargets;

  // Current frame (capped by MAX_FRAMES_IN_FLIGHT)
  int m_currentFrame = 0;
  // Index of the current swapchain image
  uint32_t m_currentImageIndex;

  VkSwapchainKHR m_swapchain{VK_NULL_HANDLE};
  VkFormat m_swapchainImageFormat;
  VkExtent2D m_swapchainExtent;
  fstl::fixed_vector<VkImage> m_swapchainImages;
  fstl::fixed_vector<VkImageView> m_swapchainImageViews;

  VkRenderPass m_renderPass{VK_NULL_HANDLE};
  VkRenderPass m_imguiRenderPass{VK_NULL_HANDLE};

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

  void
  regenFramebuffer(VkFramebuffer &framebuffer, VkImageView &swapchainImageView);

  void regenImguiFramebuffer(
      VkFramebuffer &framebuffer, VkImageView &swapchainImageView);

  void destroyResizables();

  uint32_t
  getSwapchainNumImages(const VkSurfaceCapabilitiesKHR &surfaceCapabilities);
  VkSurfaceFormatKHR
  getSwapchainFormat(const fstl::fixed_vector<VkSurfaceFormatKHR> &formats);
  VkExtent2D getSwapchainExtent(
      uint32_t width,
      uint32_t height,
      const VkSurfaceCapabilitiesKHR &surfaceCapabilities);
  VkImageUsageFlags
  getSwapchainUsageFlags(const VkSurfaceCapabilitiesKHR &surfaceCapabilities);
  VkSurfaceTransformFlagBitsKHR
  getSwapchainTransform(const VkSurfaceCapabilitiesKHR &surfaceCapabilities);
  VkPresentModeKHR getSwapchainPresentMode(
      const fstl::fixed_vector<VkPresentModeKHR> &presentModes);
};
} // namespace vkr
