#include "commandbuffer.hpp"

#include "buffer.hpp"
#include "pipeline.hpp"

using namespace vkr;

void CommandBuffer::bindVertexBuffers(const ArrayProxy<const Buffer> &buffers) {
  SmallVec<vk::Buffer> handles;
  handles.resize(buffers.size());
  SmallVec<vk::DeviceSize> offsets;
  offsets.resize(buffers.size());
  for (size_t i = 0; i < buffers.size(); i++) {
    handles[i] = (buffers.begin()+i)->getHandle();
    offsets[i] = 0;
  }

  this->commandBuffer.bindVertexBuffers(
      0, handles.size(), handles.data(), offsets.data());
}

void CommandBuffer::bindIndexBuffer(
    Buffer &buffer, DeviceSize offset, IndexType indexType) {
  this->commandBuffer.bindIndexBuffer(buffer.getHandle(), offset, indexType);
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
