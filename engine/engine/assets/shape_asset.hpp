#pragma once

#include "../asset_manager.hpp"
#include "../shape.hpp"
#include <renderer/buffer.hpp>
#include <renderer/common.hpp>
#include <renderer/pipeline.hpp>
#include <renderer/resource_manager.hpp>
#include <vulkan/vulkan.h>

namespace engine {
class ShapeAsset : public Asset {
public:
  ShapeAsset(const Shape &shape);
  ~ShapeAsset();

  Shape m_shape;

  re_buffer_t m_vertexBuffer;
  re_buffer_t m_indexBuffer;
};
} // namespace engine
