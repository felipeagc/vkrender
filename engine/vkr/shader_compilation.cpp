#include "shader_compilation.hpp"
#include <SPIRV/GlslangToSpv.h>
#include <StandAlone/DirStackFileIncluder.h>
#include <StandAlone/ResourceLimits.h>
#include <fstream>
#include <glslang/Public/ShaderLang.h>
#include <iostream>

using namespace vkr;

bool glslangInitialized = false;

std::string getFileDir(const std::string &path) {
  size_t found = path.find_last_of("/\\");
  return path.substr(0, found);
}

std::string getFileExtension(const std::string &path) {
  const size_t pos = path.rfind('.');
  return (pos == std::string::npos) ? "" : path.substr(pos + 1);
}

EShLanguage getShaderType(const std::string &extension) {
  if (extension == "vert") {
    return EShLangVertex;
  } else if (extension == "tesc") {
    return EShLangTessControl;
  } else if (extension == "tese") {
    return EShLangTessEvaluation;
  } else if (extension == "geom") {
    return EShLangGeometry;
  } else if (extension == "frag") {
    return EShLangFragment;
  } else if (extension == "comp") {
    return EShLangCompute;
  } else {
    throw std::runtime_error("Unknown shader extension");
  }
}

std::vector<unsigned int>
vkr::compileShader(const std::string &path, const ShaderType shaderType) {
  if (!glslangInitialized) {
    glslang::InitializeProcess();
  }

  std::ifstream file(path);

  if (!file.is_open()) {
    throw std::runtime_error("Failed to open shader file " + path);
  }

  std::string inputGLSL{std::istreambuf_iterator<char>(file),
                        std::istreambuf_iterator<char>()};

  EShLanguage shaderType_;
  if (shaderType == ShaderType::eAuto) {
    shaderType_ = getShaderType(getFileExtension(path));
  } else {
    switch (shaderType) {
    case ShaderType::eVertex:
      shaderType_ = EShLangVertex;
      break;
    case ShaderType::eTessControl:
      shaderType_ = EShLangTessControl;
      break;
    case ShaderType::eTessEvaluation:
      shaderType_ = EShLangTessEvaluation;
      break;
    case ShaderType::eGeometry:
      shaderType_ = EShLangGeometry;
      break;
    case ShaderType::eFragment:
      shaderType_ = EShLangFragment;
      break;
    case ShaderType::eCompute:
      shaderType_ = EShLangCompute;
      break;
    default:
      assert(0);
      break;
    }
  }

  glslang::TShader shader(shaderType_);

  const char *inputCString = inputGLSL.c_str();
  shader.setStrings(&inputCString, 1);

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

  std::string dir = getFileDir(path);
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
    throw std::runtime_error(
        "GLSL preprocessing failed for: " + path + "\n" + shader.getInfoLog() +
        "\n" + shader.getInfoDebugLog());
  }

  const char *preprocessedCStr = preprocessedGLSL.c_str();
  shader.setStrings(&preprocessedCStr, 1);

  if (!shader.parse(&resources, 100, false, messages)) {
    throw std::runtime_error(
        "GLSL parsing failed for: " + path + "\n" + shader.getInfoLog() + "\n" +
        shader.getInfoDebugLog());
  }

  glslang::TProgram program;
  program.addShader(&shader);

  if (!program.link(messages)) {
    throw std::runtime_error(
        "GLSL linking failed for: " + path + "\n" + shader.getInfoLog() + "\n" +
        shader.getInfoDebugLog());
  }

  std::vector<unsigned int> spirv;
  spv::SpvBuildLogger logger;
  glslang::SpvOptions spvOptions;
  glslang::GlslangToSpv(
      *program.getIntermediate(shaderType_), spirv, &logger, &spvOptions);

  return spirv;
}
