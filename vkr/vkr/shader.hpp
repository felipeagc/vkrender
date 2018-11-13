#pragma once

#include <fstl/fixed_vector.hpp>
#include <fstream>
#include "vertex_format.hpp"
#include "descriptor.hpp"

namespace vkr {
class Shader {
public:
  // Creates a shader from a path to a GLSL file
  Shader(const std::string &vertexPath, const std::string &fragmentPath);

  // Creates a shader from SPIR-V code
  Shader(
      const std::vector<uint32_t> &vertexCode,
      const std::vector<uint32_t> &fragmentCode);

  ~Shader(){};
  Shader(const Shader &other) = delete;
  Shader &operator=(Shader &other) = delete;

  operator bool() { return this->vertexModule && this->fragmentModule; };

  fstl::fixed_vector<vk::PipelineShaderStageCreateInfo>
  getPipelineShaderStageCreateInfos() const;

  struct ShaderMetadata {
    fstl::fixed_vector<vkr::DescriptorSetLayoutBinding>
        descriptorSetLayoutBindings;
    vkr::VertexFormat vertexFormat;
  };

  ShaderMetadata getAutoMetadata() const;

  void destroy();

  static std::vector<uint32_t> loadCode(const std::string &path) {
    std::ifstream file(path, std::ios::binary);

    if (file.fail()) {
      throw std::runtime_error("Failed to load shader code file");
    }

    std::streampos begin, end;
    begin = file.tellg();
    file.seekg(0, std::ios::end);
    end = file.tellg();

    std::vector<uint32_t> result(static_cast<size_t>(end - begin) / 4);

    file.seekg(0, std::ios::beg);
    file.read(reinterpret_cast<char *>(result.data()), end - begin);
    file.close();

    return result;
  }

private:
  std::vector<uint32_t> vertexCode;
  std::vector<uint32_t> fragmentCode;
  vk::ShaderModule vertexModule;
  vk::ShaderModule fragmentModule;

  vk::ShaderModule createShaderModule(const std::vector<uint32_t> &code) const;
};
}
