#pragma once

#include <fstl/fixed_vector.hpp>

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
      const fstl::fixed_vector<VkDescriptorSetLayout> &descriptorSetLayouts =
          {});
  ~GraphicsPipeline(){};
  GraphicsPipeline(const GraphicsPipeline &) = delete;
  GraphicsPipeline &operator=(GraphicsPipeline &) = delete;
  GraphicsPipeline(GraphicsPipeline &&) = default;
  GraphicsPipeline &operator=(GraphicsPipeline &&) = default;

  operator bool() { return this->pipeline_; };

  VkPipelineLayout getLayout() const;
  VkPipeline getPipeline() const;

  void destroy();

private:
  VkPipeline pipeline_;
  VkPipelineLayout pipelineLayout_;
};
} // namespace vkr
