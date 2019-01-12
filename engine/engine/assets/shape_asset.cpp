#include "shape_asset.hpp"

using namespace engine;

ShapeAsset::ShapeAsset(const Shape &shape) : m_shape(shape) {
  size_t vertexBufferSize = sizeof(re_vertex_t) * m_shape.m_vertices.size();
  size_t indexBufferSize = sizeof(uint32_t) * m_shape.m_indices.size();

  re_buffer_t staging_buffer;
  re_buffer_init_staging(
      &staging_buffer,
      vertexBufferSize > indexBufferSize ? vertexBufferSize : indexBufferSize);

  void *staging_pointer;
  re_buffer_map_memory(&staging_buffer, &staging_pointer);

  re_buffer_init_vertex(&m_vertexBuffer, vertexBufferSize);
  re_buffer_init_index(&m_indexBuffer, indexBufferSize);

  memcpy(staging_pointer, m_shape.m_vertices.data(), vertexBufferSize);
  re_buffer_transfer_to_buffer(
      &staging_buffer, &m_vertexBuffer, vertexBufferSize);

  memcpy(staging_pointer, m_shape.m_indices.data(), indexBufferSize);
  re_buffer_transfer_to_buffer(
      &staging_buffer, &m_indexBuffer, indexBufferSize);

  re_buffer_unmap_memory(&staging_buffer);
  re_buffer_destroy(&staging_buffer);
}

ShapeAsset::~ShapeAsset() {
  re_buffer_destroy(&m_vertexBuffer);
  re_buffer_destroy(&m_indexBuffer);
}
