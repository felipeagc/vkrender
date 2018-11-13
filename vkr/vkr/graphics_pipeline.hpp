#pragma once

#include <fstl/fixed_vector.hpp>
#include "aliases.hpp"
#include "descriptor.hpp"

namespace vkr {
class Window;
class Shader;
class VertexFormat;

class GraphicsPipeline {
  friend class CommandBuffer;

public:
  GraphicsPipeline(
      const Window &window,
      const Shader &shader,
      const VertexFormat &vertexFormat,
      const fstl::fixed_vector<vkr::DescriptorSetLayout> &descriptorSetLayouts = {});
  ~GraphicsPipeline(){};
  GraphicsPipeline(const GraphicsPipeline &) = delete;
  GraphicsPipeline &operator=(GraphicsPipeline &) = delete;
  GraphicsPipeline(GraphicsPipeline &&) = default;
  GraphicsPipeline &operator=(GraphicsPipeline &&) = default;

  operator bool() { return this->pipeline; };

  PipelineLayout getLayout() const;

  void destroy();

private:
  vk::Pipeline pipeline;
  PipelineLayout pipelineLayout;
};
} // namespace vkr
