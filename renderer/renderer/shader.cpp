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

enum shader_type_t {
  SHADER_TYPE_VERTEX,
  SHADER_TYPE_TESS_CONTROL,
  SHADER_TYPE_TESS_EVALUATION,
  SHADER_TYPE_GEOMETRY,
  SHADER_TYPE_FRAGMENT,
  SHADER_TYPE_COMPUTE,
};

static inline uint32_t *load_spv_code(const char *path, size_t *codeSize) {
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

static inline uint32_t *compile_glsl(
    const char *glsl_code, const shader_type_t shader_type, size_t *code_size) {
  if (!glslangInitialized) {
    glslang::InitializeProcess();
  }

  EShLanguage shaderType_;
  switch (shader_type) {
  case SHADER_TYPE_VERTEX:
    shaderType_ = EShLangVertex;
    break;
  case SHADER_TYPE_TESS_CONTROL:
    shaderType_ = EShLangTessControl;
    break;
  case SHADER_TYPE_TESS_EVALUATION:
    shaderType_ = EShLangTessEvaluation;
    break;
  case SHADER_TYPE_GEOMETRY:
    shaderType_ = EShLangGeometry;
    break;
  case SHADER_TYPE_FRAGMENT:
    shaderType_ = EShLangFragment;
    break;
  case SHADER_TYPE_COMPUTE:
    shaderType_ = EShLangCompute;
    break;
  default:
    assert(0);
    break;
  }

  glslang::TShader shader(shaderType_);

  shader.setStrings(&glsl_code, 1);

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

  // includer.pushExternalLocalDirectory(dir);

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
        "GLSL preprocessing failed: %s\n%s",
        shader.getInfoLog(),
        shader.getInfoDebugLog());
    assert(0);
  }

  const char *preprocessedCStr = preprocessedGLSL.c_str();
  shader.setStrings(&preprocessedCStr, 1);

  if (!shader.parse(&resources, 100, false, messages)) {
    ftl::error(
        "GLSL parsing failed: %s\n%s",
        shader.getInfoLog(),
        shader.getInfoDebugLog());
    assert(0);
  }

  glslang::TProgram program;
  program.addShader(&shader);

  if (!program.link(messages)) {
    ftl::error(
        "GLSL linking failed: %s\n%s",
        shader.getInfoLog(),
        shader.getInfoDebugLog());
    assert(0);
  }

  std::vector<unsigned int> spirv;
  spv::SpvBuildLogger logger;
  glslang::SpvOptions spvOptions;
  glslang::GlslangToSpv(
      *program.getIntermediate(shaderType_), spirv, &logger, &spvOptions);

  *code_size = spirv.size() * sizeof(uint32_t);

  uint32_t *code = new uint32_t[spirv.size()];
  memcpy(code, spirv.data(), *code_size);
  return code;
}

static inline VkShaderModule
create_shader_module(const uint32_t *code, size_t code_size) {
  VkShaderModule module;

  VkShaderModuleCreateInfo createInfo = {
      VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO, nullptr, 0, code_size, code};

  VK_CHECK(vkCreateShaderModule(ctx().m_device, &createInfo, nullptr, &module));
  return module;
}

void re_shader_init_glsl(
    re_shader_t *shader,
    const char *vertex_glsl_code,
    const char *fragment_glsl_code) {
  size_t vertex_spirv_code_size, fragment_spirv_code_size;

  uint32_t *vertex_spirv_code = compile_glsl(
      vertex_glsl_code, SHADER_TYPE_VERTEX, &vertex_spirv_code_size);
  uint32_t *fragment_spirv_code = compile_glsl(
      fragment_glsl_code, SHADER_TYPE_FRAGMENT, &fragment_spirv_code_size);

  shader->vertex_module =
      create_shader_module(vertex_spirv_code, vertex_spirv_code_size);
  shader->fragment_module =
      create_shader_module(fragment_spirv_code, fragment_spirv_code_size);

  delete[] vertex_spirv_code;
  delete[] fragment_spirv_code;
}

void re_shader_init_spirv(
    re_shader_t *shader,
    const uint32_t *vertex_code,
    size_t vertex_code_size,
    const uint32_t *fragment_code,
    size_t fragment_code_size) {
  shader->vertex_module = create_shader_module(vertex_code, vertex_code_size);
  shader->fragment_module =
      create_shader_module(fragment_code, fragment_code_size);
}

void re_shader_destroy(re_shader_t *shader) {
  VK_CHECK(vkDeviceWaitIdle(ctx().m_device));

  if (shader->vertex_module != VK_NULL_HANDLE &&
      shader->fragment_module != VK_NULL_HANDLE) {
    vkDestroyShaderModule(ctx().m_device, shader->vertex_module, nullptr);
    vkDestroyShaderModule(ctx().m_device, shader->fragment_module, nullptr);
    shader->vertex_module = VK_NULL_HANDLE;
    shader->fragment_module = VK_NULL_HANDLE;
  }
}

void re_shader_get_pipeline_stages(
    const re_shader_t *shader,
    VkPipelineShaderStageCreateInfo *pipeline_stages) {
  pipeline_stages[0] = {
      VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, // sType
      NULL,                                                // pNext
      0,                                                   // flags
      VK_SHADER_STAGE_VERTEX_BIT,                          // stage
      shader->vertex_module,                               // module
      "main",                                              // pName
      NULL, // pSpecializationInfo
  };
  pipeline_stages[1] = {
      VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, // sType
      NULL,                                                // pNext
      0,                                                   // flags
      VK_SHADER_STAGE_FRAGMENT_BIT,                        // stage
      shader->fragment_module,                             // module
      "main",                                              // pName
      NULL, // pSpecializationInfo
  };
}
