#include "shader.hpp"
#include "context.hpp"
#include "util.hpp"
#include <SPIRV/GlslangToSpv.h>
#include <StandAlone/DirStackFileIncluder.h>
#include <StandAlone/ResourceLimits.h>
#include <cstdio>
#include <cstring>
#include <ftl/logging.hpp>
#include <glslang/Public/ShaderLang.h>

using namespace renderer;

static bool glslangInitialized = false;

enum class ShaderType {
  VERTEX,
  TESS_CONTROL,
  TESS_EVALUATION,
  GEOMETRY,
  FRAGMENT,
  COMPUTE,
};

static inline uint32_t *loadSPVCode(const char *path, size_t *codeSize) {
  FILE *file = fopen(path, "rb");

  if (file == nullptr) {
    return nullptr;
  }

  fseek(file, 0, SEEK_END);
  *codeSize = ftell(file);
  fseek(file, 0, SEEK_SET);

  uint32_t *storage = new uint32_t[*codeSize / sizeof(uint32_t)];

  fread(storage, sizeof(uint32_t), *codeSize / sizeof(uint32_t), file);

  fclose(file);

  return storage;
}

void getFileDir(const char *path, char *outDir) {
  char *delim = strrchr(path, '/');
  if (delim == nullptr) {
    delim = strrchr(path, '\\');
  }
  if (delim == nullptr) {
    return;
  }
  int i = 0;
  for (const char *c = path; c <= delim; c++) {
    outDir[i++] = *c;
  }
  outDir[i] = '\0';
}

static inline std::vector<uint32_t>
compileGLSL(const char *path, const ShaderType shaderType) {
  if (!glslangInitialized) {
    glslang::InitializeProcess();
  }

  FILE *file = fopen(path, "r");

  if (file == nullptr) {
    ftl::error("Could not open shader file: %s", path);
    assert(0);
  }

  fseek(file, 0, SEEK_END);
  size_t strSize = ftell(file);
  fseek(file, 0, SEEK_SET);

  char *buffer = new char[strSize + 1];
  fread(buffer, sizeof(char), strSize, file);
  buffer[strSize] = '\0';

  fclose(file);

  EShLanguage shaderType_;
  switch (shaderType) {
  case ShaderType::VERTEX:
    shaderType_ = EShLangVertex;
    break;
  case ShaderType::TESS_CONTROL:
    shaderType_ = EShLangTessControl;
    break;
  case ShaderType::TESS_EVALUATION:
    shaderType_ = EShLangTessEvaluation;
    break;
  case ShaderType::GEOMETRY:
    shaderType_ = EShLangGeometry;
    break;
  case ShaderType::FRAGMENT:
    shaderType_ = EShLangFragment;
    break;
  case ShaderType::COMPUTE:
    shaderType_ = EShLangCompute;
    break;
  default:
    assert(0);
    break;
  }

  glslang::TShader shader(shaderType_);

  shader.setStrings(&buffer, 1);

  int clientInputSemanticsVersion = 100;

  auto vulkanClientVersion = glslang::EShTargetVulkan_1_0;
  auto targetVersion = glslang::EShTargetSpv_1_0;

  shader.setEnvInput(
      glslang::EShSourceGlsl,
      shaderType_,
      glslang::EShClientVulkan,
      clientInputSemanticsVersion);
  shader.setEnvClient(glslang::EShClientVulkan, vulkanClientVersion);
  shader.setEnvTarget(glslang::EshTargetSpv, targetVersion);

  TBuiltInResource resources = glslang::DefaultTBuiltInResource;
  EShMessages messages = (EShMessages)(EShMsgSpvRules | EShMsgVulkanRules);

  const int defaultVersion = 100;

  DirStackFileIncluder includer;

  char dir[256];
  getFileDir(path, dir);
  includer.pushExternalLocalDirectory(dir);

  std::string preprocessedGLSL;

  if (!shader.preprocess(
          &resources,
          defaultVersion,
          ENoProfile,
          false,
          false,
          messages,
          &preprocessedGLSL,
          includer)) {
    ftl::error(
        "GLSL preprocessing failed for: %s\n%s\n%s",
        path,
        shader.getInfoLog(),
        shader.getInfoDebugLog());
    delete[] buffer;
    assert(0);
  }

  const char *preprocessedCStr = preprocessedGLSL.c_str();
  shader.setStrings(&preprocessedCStr, 1);

  if (!shader.parse(&resources, 100, false, messages)) {
    ftl::error(
        "GLSL parsing failed for: %s\n%s\n%s",
        path,
        shader.getInfoLog(),
        shader.getInfoDebugLog());
    delete[] buffer;
    assert(0);
  }

  glslang::TProgram program;
  program.addShader(&shader);

  if (!program.link(messages)) {
    ftl::error(
        "GLSL linking failed for: %s\n%s\n%s",
        path,
        shader.getInfoLog(),
        shader.getInfoDebugLog());
    delete[] buffer;
    assert(0);
  }

  std::vector<unsigned int> spirv;
  spv::SpvBuildLogger logger;
  glslang::SpvOptions spvOptions;
  glslang::GlslangToSpv(
      *program.getIntermediate(shaderType_), spirv, &logger, &spvOptions);

  delete[] buffer;

  return spirv;
}

static inline VkShaderModule
createShaderModule(size_t codeSize, uint32_t *code) {
  VkShaderModule shaderModule;

  VkShaderModuleCreateInfo createInfo = {
      VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO, nullptr, 0, codeSize, code};

  VK_CHECK(vkCreateShaderModule(
      ctx().m_device, &createInfo, nullptr, &shaderModule));
  return shaderModule;
}

Shader::Shader(const char *vertexPath, const char *fragmentPath) {
  bool isGLSL = false;
  const char *spvExt = ".spv";
  size_t vertexPathLength = strlen(vertexPath);
  for (size_t i = 0; i < strlen(spvExt); i++) {
    if (vertexPath[vertexPathLength - i] != spvExt[strlen(spvExt) - i]) {
      isGLSL = true;
      break;
    }
  }

  if (isGLSL) {
    std::vector<uint32_t> vertexCode =
        compileGLSL(vertexPath, ShaderType::VERTEX);
    std::vector<uint32_t> fragmentCode =
        compileGLSL(fragmentPath, ShaderType::FRAGMENT);
    m_vertexModule = createShaderModule(
        vertexCode.size() * sizeof(uint32_t), vertexCode.data());
    m_fragmentModule = createShaderModule(
        fragmentCode.size() * sizeof(uint32_t), fragmentCode.data());
  } else {
    size_t vertexCodeSize, fragmentCodeSize;
    uint32_t *vertexCode = loadSPVCode(vertexPath, &vertexCodeSize);
    uint32_t *fragmentCode = loadSPVCode(fragmentPath, &fragmentCodeSize);
    m_vertexModule = createShaderModule(vertexCodeSize, vertexCode);
    m_fragmentModule = createShaderModule(fragmentCodeSize, fragmentCode);
    delete[] vertexCode;
    delete[] fragmentCode;
  }
}

Shader::Shader(
    size_t vertexCodeSize,
    uint32_t *vertexCode,
    size_t fragmentCodeSize,
    uint32_t *fragmentCode) {
  m_vertexModule = createShaderModule(vertexCodeSize, vertexCode);
  m_fragmentModule = createShaderModule(fragmentCodeSize, fragmentCode);
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
