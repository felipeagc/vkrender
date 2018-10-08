#pragma once

#include "util.hpp"
#include <vulkan/vulkan.hpp>

namespace vkr {
class Buffer;
class GraphicsPipeline;
class DescriptorSet;

class CommandBuffer {
public:
  CommandBuffer(vk::CommandBuffer commandBuffer)
      : commandBuffer(commandBuffer){};
  ~CommandBuffer(){};

  void bindVertexBuffer(Buffer &buffer);
  void bindIndexBuffer(Buffer &buffer, DeviceSize offset, IndexType indexType);
  void bindDescriptorSets(
      PipelineBindPoint pipelineBindPoint,
      PipelineLayout layout,
      uint32_t firstSet,
      ArrayProxy<const DescriptorSet> descriptorSets,
      ArrayProxy<const uint32_t> dynamicOffsets);
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
  vk::CommandBuffer commandBuffer;
};
} // namespace vkr
