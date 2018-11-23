#pragma once

#include "gltf_model.hpp"

namespace vkr {
struct GltfModelInstance {
  GltfModelInstance(GltfModel &model);

  GltfModel model;
};
} // namespace vkr
