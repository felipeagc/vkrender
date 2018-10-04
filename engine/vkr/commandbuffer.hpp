#pragma once

#include <vulkan/vulkan.hpp>
#include "util.hpp"

namespace vkr {
class Buffer;
class GraphicsPipeline;

class CommandBuffer {
public:
  CommandBuffer(vk::CommandBuffer& commandBuffer)
      : commandBuffer(commandBuffer){};
  ~CommandBuffer(){};

  void bindVertexBuffers(ArrayProxy<const Buffer> buffers);
  void bindGraphicsPipeline(GraphicsPipeline &pipeline);

  void draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance);

private:
  vk::CommandBuffer& commandBuffer;
};
} // namespace vkr
