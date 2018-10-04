#include "commandbuffer.hpp"

#include "buffer.hpp"
#include "pipeline.hpp"

using namespace vkr;

void CommandBuffer::bindVertexBuffers(ArrayProxy<const vkr::Buffer> buffers) {
  for (auto &buffer : buffers) {
    // TODO: find a way to do this without multiple bind calls
    this->commandBuffer.bindVertexBuffers(0, buffer.buffer, {0});
  }
}

void CommandBuffer::bindIndexBuffer(
    Buffer &buffer, DeviceSize offset, IndexType indexType) {
  this->commandBuffer.bindIndexBuffer(buffer.buffer, offset, indexType);
}

void CommandBuffer::bindGraphicsPipeline(vkr::GraphicsPipeline &pipeline) {
  this->commandBuffer.bindPipeline(
      vk::PipelineBindPoint::eGraphics, pipeline.pipeline);
}

void CommandBuffer::draw(
    uint32_t vertexCount,
    uint32_t instanceCount,
    uint32_t firstVertex,
    uint32_t firstInstance) {
  this->commandBuffer.draw(
      vertexCount, instanceCount, firstVertex, firstInstance);
}

void CommandBuffer::drawIndexed(
    uint32_t indexCount,
    uint32_t instanceCount,
    uint32_t firstIndex,
    int32_t vertexOffset,
    uint32_t firstInstance) {
  this->commandBuffer.drawIndexed(
      indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
}
