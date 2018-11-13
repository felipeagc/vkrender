#pragma once

#include "aliases.hpp"
#include <vulkan/vulkan.hpp>
#include <fstl/array_proxy.hpp>

namespace vkr {
class Buffer;
class GraphicsPipeline;

class CommandBuffer {
public:
  CommandBuffer(vk::CommandBuffer commandBuffer)
      : commandBuffer_(commandBuffer){};
  ~CommandBuffer(){};

  void bindVertexBuffers(const fstl::array_proxy<const Buffer> &buffers);
  void bindIndexBuffer(Buffer &buffer, DeviceSize offset, IndexType indexType);
  void bindDescriptorSets(
      PipelineBindPoint pipelineBindPoint,
      PipelineLayout layout,
      uint32_t firstSet,
      fstl::array_proxy<const DescriptorSet> descriptorSets,
      fstl::array_proxy<const uint32_t> dynamicOffsets);
  void bindGraphicsPipeline(GraphicsPipeline &pipeline);

  void draw(
      uint32_t vertexCount,
      uint32_t instanceCount,
      uint32_t firstVertex,
      uint32_t firstInstance);
  void drawIndexed(
      uint32_t indexCount,
      uint32_t instanceCount,
      uint32_t firstIndex,
      int32_t vertexOffset,
      uint32_t firstInstance);

private:
  vk::CommandBuffer commandBuffer_;
};
} // namespace vkr
