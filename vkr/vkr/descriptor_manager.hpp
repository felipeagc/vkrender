#pragma once

#include "descriptor.hpp"
#include <utility>
#include <vector>

namespace vkr {
const char* const DESC_CAMERA = "camera";
const char* const DESC_MESH = "mesh";
const char* const DESC_MATERIAL = "material";
const char* const DESC_LIGHTING = "lighting";
const char* const DESC_IMGUI = "imgui";

class DescriptorManager {
  friend class Context;

public:
  std::pair<DescriptorPool *, DescriptorSetLayout *>
  operator[](const std::string &key);

  DescriptorPool *getPool(const std::string &key);
  DescriptorSetLayout *getSetLayout(const std::string &key);

  // Returns true if successful
  // Returns false if a key with that name already exists
  bool addPool(const std::string &key, DescriptorPool pool);
  bool addSetLayout(const std::string &key, DescriptorSetLayout setLayout);

  fstl::fixed_vector<DescriptorSetLayout> getDefaultSetLayouts();

protected:
  void init();
  void destroy();

  std::vector<std::pair<std::string, DescriptorPool>> pools_;
  std::vector<std::pair<std::string, DescriptorSetLayout>> setLayouts_;
};
} // namespace vkr
