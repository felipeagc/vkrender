#pragma once

#include <vulkan/vulkan.h>

namespace renderer {
class BaseRenderTarget {
public:
  inline VkSampleCountFlagBits getSampleCount() const { return m_sampleCount; };
  inline VkRenderPass getRenderPass() const { return m_renderPass; };

protected:
  VkSampleCountFlagBits m_sampleCount = VK_SAMPLE_COUNT_1_BIT;
  VkRenderPass m_renderPass = VK_NULL_HANDLE;
};
} // namespace renderer
