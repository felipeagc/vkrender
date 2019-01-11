#pragma once

#include <ftl/vector.hpp>

namespace renderer {
class Shader {
public:
  // Creates a shader from a path to a GLSL file
  Shader(const char *vertexPath, const char *fragmentPath);

  // Creates a shader from SPIR-V code
  Shader(
      size_t vertexCodeSize,
      uint32_t *vertexCode,
      size_t fragmentCodeSize,
      uint32_t *fragmentCode);

  ~Shader();

  Shader(const Shader &other) = delete;
  Shader &operator=(const Shader &other) = delete;

  Shader(Shader &&other);
  Shader &operator=(Shader &&other);

  ftl::small_vector<VkPipelineShaderStageCreateInfo>
  getPipelineShaderStageCreateInfos() const;

private:
  VkShaderModule m_vertexModule;
  VkShaderModule m_fragmentModule;
};
} // namespace renderer
