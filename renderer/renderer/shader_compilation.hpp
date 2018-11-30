#pragma once

#include <string>
#include <vector>

namespace renderer {
enum class ShaderType {
  eVertex,
  eTessControl,
  eTessEvaluation,
  eGeometry,
  eFragment,
  eCompute,
  eAuto
};

std::vector<unsigned int> compileShader(
    const std::string &path, const ShaderType shaderType = ShaderType::eAuto);
} // namespace renderer
