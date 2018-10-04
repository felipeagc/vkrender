#pragma once

#include "util.hpp"
#include <vulkan/vulkan.hpp>

namespace vkr {
class Buffer;
class GraphicsPipeline;

class CommandBuffer {
public:
  CommandBuffer(vk::CommandBuffer &commandBuffer)
      : commandBuffer(commandBuffer){};
  ~CommandBuffer(){};

  void bindVertexBuffers(ArrayProxy<const Buffer> buffers);
  void bindIndexBuffer(Buffer &buffer, DeviceSize offset, IndexType indexType);
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
  vk::CommandBuffer &commandBuffer;
};
} // namespace vkr
