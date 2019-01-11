#pragma once

#include <fstream>
#include <ftl/vector.hpp>
#include <vector>

namespace renderer {
class Shader {
public:
  // Creates a shader from a path to a GLSL file
  Shader(const std::string &vertexPath, const std::string &fragmentPath);

  // Creates a shader from SPIR-V code
  Shader(
      const std::vector<uint32_t> &vertexCode,
      const std::vector<uint32_t> &fragmentCode);

  ~Shader();

  Shader(const Shader &other) = delete;
  Shader &operator=(const Shader &other) = delete;

  Shader(Shader &&other);
  Shader &operator=(Shader &&other);

  operator bool() { return m_vertexModule && m_fragmentModule; };

  ftl::small_vector<VkPipelineShaderStageCreateInfo>
  getPipelineShaderStageCreateInfos() const;

private:
  VkShaderModule m_vertexModule;
  VkShaderModule m_fragmentModule;

  VkShaderModule createShaderModule(const std::vector<uint32_t> &code) const;
};
} // namespace renderer
