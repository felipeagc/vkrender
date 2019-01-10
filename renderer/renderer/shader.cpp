#include "shader.hpp"
#include "context.hpp"
#include "shader_compilation.hpp"
#include "util.hpp"
#include <ftl/logging.hpp>
#include <spirv_reflect.hpp>

using namespace renderer;

static inline std::vector<uint32_t> loadSPVCode(const std::string &path) {
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

Shader::Shader(const std::string &vertexPath, const std::string &fragmentPath) {
  bool isGLSL = false;
  const char *spvExt = ".spv";
  for (size_t i = 0; i < strlen(spvExt); i++) {
    if (vertexPath[vertexPath.length() - i] != spvExt[strlen(spvExt) - i]) {
      isGLSL = true;
      break;
    }
  }

  if (isGLSL) {
    auto vertexCode = compileShader(vertexPath, ShaderType::eVertex);
    auto fragmentCode = compileShader(fragmentPath, ShaderType::eFragment);
    m_vertexModule = this->createShaderModule(vertexCode);
    m_fragmentModule = this->createShaderModule(fragmentCode);
  } else {
    auto vertexCode = loadSPVCode(vertexPath);
    auto fragmentCode = loadSPVCode(fragmentPath);
    m_vertexModule = this->createShaderModule(vertexCode);
    m_fragmentModule = this->createShaderModule(fragmentCode);
  }
}

Shader::Shader(
    const std::vector<uint32_t> &vertexCode,
    const std::vector<uint32_t> &fragmentCode) {
  m_vertexModule = this->createShaderModule(vertexCode);
  m_fragmentModule = this->createShaderModule(fragmentCode);
}

Shader::~Shader() {
  VK_CHECK(vkDeviceWaitIdle(ctx().m_device));

  if (m_vertexModule != VK_NULL_HANDLE && m_fragmentModule != VK_NULL_HANDLE) {
    vkDestroyShaderModule(ctx().m_device, m_vertexModule, nullptr);
    vkDestroyShaderModule(ctx().m_device, m_fragmentModule, nullptr);
  }
}

Shader::Shader(Shader &&other) {
  m_vertexModule = other.m_vertexModule;
  m_fragmentModule = other.m_fragmentModule;

  other.m_vertexModule = VK_NULL_HANDLE;
  other.m_fragmentModule = VK_NULL_HANDLE;
}

Shader &Shader::operator=(Shader &&other) {
  VK_CHECK(vkDeviceWaitIdle(ctx().m_device));

  if (m_vertexModule != VK_NULL_HANDLE && m_fragmentModule != VK_NULL_HANDLE) {
    vkDestroyShaderModule(ctx().m_device, m_vertexModule, nullptr);
    vkDestroyShaderModule(ctx().m_device, m_fragmentModule, nullptr);
  }

  m_vertexModule = other.m_vertexModule;
  m_fragmentModule = other.m_fragmentModule;

  other.m_vertexModule = VK_NULL_HANDLE;
  other.m_fragmentModule = VK_NULL_HANDLE;

  return *this;
}

ftl::small_vector<VkPipelineShaderStageCreateInfo>
Shader::getPipelineShaderStageCreateInfos() const {
  return ftl::small_vector<VkPipelineShaderStageCreateInfo>{
      {
          VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, // sType
          nullptr,                                             // pNext
          0,                                                   // flags
          VK_SHADER_STAGE_VERTEX_BIT,                          // stage
          m_vertexModule,                                      // module
          "main",                                              // pName
          nullptr, // pSpecializationInfo
      },
      {
          VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, // sType
          nullptr,                                             // pNext
          0,                                                   // flags
          VK_SHADER_STAGE_FRAGMENT_BIT,                        // stage
          m_fragmentModule,                                    // module
          "main",                                              // pName
          nullptr, // pSpecializationInfo
      },
  };
}

VkShaderModule
Shader::createShaderModule(const std::vector<uint32_t> &code) const {
  VkShaderModule shaderModule;

  VkShaderModuleCreateInfo createInfo = {
      VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
      nullptr,
      0,
      code.size() * sizeof(uint32_t),
      code.data()};

  VK_CHECK(vkCreateShaderModule(
      ctx().m_device, &createInfo, nullptr, &shaderModule));
  return shaderModule;
}
