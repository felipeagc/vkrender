#include "shape_asset.hpp"

using namespace engine;

ShapeAsset::ShapeAsset(const Shape &shape) : m_shape(shape) {
  size_t vertexBufferSize =
      sizeof(renderer::StandardVertex) * m_shape.m_vertices.size();
  size_t indexBufferSize = sizeof(uint32_t) * m_shape.m_indices.size();

  renderer::Buffer stagingBuffer{renderer::BufferType::eStaging,
                                 std::max(vertexBufferSize, indexBufferSize)};

  void *stagingMemoryPointer;
  stagingBuffer.mapMemory(&stagingMemoryPointer);

  this->m_vertexBuffer =
      renderer::Buffer{renderer::BufferType::eVertex, vertexBufferSize};

  this->m_indexBuffer =
      renderer::Buffer{renderer::BufferType::eIndex, indexBufferSize};

  memcpy(stagingMemoryPointer, m_shape.m_vertices.data(), vertexBufferSize);
  stagingBuffer.bufferTransfer(m_vertexBuffer, vertexBufferSize);

  memcpy(stagingMemoryPointer, m_shape.m_indices.data(), indexBufferSize);
  stagingBuffer.bufferTransfer(m_indexBuffer, indexBufferSize);

  stagingBuffer.unmapMemory();
  stagingBuffer.destroy();
}

ShapeAsset::~ShapeAsset() {
  m_vertexBuffer.destroy();
  m_indexBuffer.destroy();
}
