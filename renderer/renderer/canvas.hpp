#pragma once

#include "common.hpp"
#include "render_target.hpp"
#include "resource_manager.hpp"
#include <vulkan/vk_mem_alloc.h>
#include <vulkan/vulkan.h>

namespace renderer {
class Window;
class GraphicsPipeline;

class Canvas : public RenderTarget {
public:
  Canvas(
      const uint32_t width,
      const uint32_t height,
      const VkFormat colorFormat = VK_FORMAT_R8G8B8A8_UNORM);
  ~Canvas();

  // RenderTarget cannot be copied
  Canvas(const Canvas &) = delete;
  Canvas &operator=(const Canvas &) = delete;

  void beginRenderPass(
      const VkCommandBuffer commandBuffer, uint32_t resourceIndex = 0);
  void endRenderPass(const VkCommandBuffer commandBuffer);

  void beginRenderPass(Window &window);
  void endRenderPass(Window &window);

  void draw(Window &window, GraphicsPipeline &pipeline);

  void resize(const uint32_t width, const uint32_t height);

  VkImage getColorImage(uint32_t resourceIndex = 0);

protected:
  uint32_t m_width = 0;
  uint32_t m_height = 0;

  VkFormat m_depthFormat;
  VkFormat m_colorFormat;

  // @note: we could have multiple of these resources per frame-in-flight,
  // but from initial testing there's no real performance gain
  struct {
    struct {
      VkImage image = VK_NULL_HANDLE;
      VmaAllocation allocation = VK_NULL_HANDLE;
      // @todo: no need for multiple samplers
      VkSampler sampler = VK_NULL_HANDLE;
      VkImageView imageView = VK_NULL_HANDLE;
    } color;

    struct {
      VkImage image = VK_NULL_HANDLE;
      VmaAllocation allocation = VK_NULL_HANDLE;
      VkImageView imageView = VK_NULL_HANDLE;
    } depth;

    VkFramebuffer framebuffer = VK_NULL_HANDLE;

    // For rendering this render target's image to another render target
    ResourceSet resourceSet;
  } m_resources[1];

private:
  void createColorTarget();
  void destroyColorTarget();

  void createDepthTarget();
  void destroyDepthTarget();

  void createDescriptorSet();
  void destroyDescriptorSet();

  void createFramebuffers();
  void destroyFramebuffers();

  void createRenderPass();
  void destroyRenderPass();
};
} // namespace renderer
