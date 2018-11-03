#pragma once

#include <string>
#include <vector>

namespace vkr {
enum ShaderType {
  eVertex,
  eTessControl,
  eTessEvaluation,
  eGeometry,
  eFragment,
  eCompute,
  eAuto
};

std::vector<unsigned int>
compileShader(const std::string &path, const ShaderType shaderType = ShaderType::eAuto);
} // namespace vkr
