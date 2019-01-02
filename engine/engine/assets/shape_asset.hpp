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
  renderer::Buffer m_vertexBuffer;
  renderer::Buffer m_indexBuffer;
};
} // namespace engine
