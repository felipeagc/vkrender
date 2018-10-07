#include "commandbuffer.hpp"

#include "buffer.hpp"
#include "pipeline.hpp"

using namespace vkr;

void CommandBuffer::bindVertexBuffers(ArrayProxy<const vkr::Buffer> buffers) {
  for (auto &buffer : buffers) {
    // TODO: find a way to do this without multiple bind calls
    // TODO: WTF this is wrong it replaces the old binding
    this->commandBuffer.bindVertexBuffers(0, buffer.getVkBuffer(), {0});
  }
}

void CommandBuffer::bindIndexBuffer(
    Buffer &buffer, DeviceSize offset, IndexType indexType) {
  this->commandBuffer.bindIndexBuffer(buffer.getVkBuffer(), offset, indexType);
}

void CommandBuffer::bindDescriptorSets(
    PipelineBindPoint pipelineBindPoint,
    PipelineLayout layout,
    uint32_t firstSet,
    ArrayProxy<const DescriptorSet> descriptorSets,
    ArrayProxy<const uint32_t> dynamicOffsets) {
  this->commandBuffer.bindDescriptorSets(
      pipelineBindPoint,
      layout,
      firstSet,
      descriptorSets.size(),
      descriptorSets.data(),
      dynamicOffsets.size(),
      dynamicOffsets.data());
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
