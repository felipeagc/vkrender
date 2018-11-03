#pragma once

#include "pipeline.hpp"
#include <utility>
#include <vector>

namespace vkr {
const std::string DESC_CAMERA = "camera";
const std::string DESC_MESH = "mesh";
const std::string DESC_MATERIAL = "material";
const std::string DESC_LIGHTING = "lighting";

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

protected:
  void init();
  void destroy();

  std::vector<std::pair<std::string, DescriptorPool>> pools;
  std::vector<std::pair<std::string, DescriptorSetLayout>> setLayouts;
};
} // namespace vkr
