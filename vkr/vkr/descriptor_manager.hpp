#pragma once

#include <fstl/fixed_vector.hpp>
#include <utility>
#include <vector>

namespace vkr {
const char *const DESC_CAMERA = "camera";
const char *const DESC_MESH = "mesh";
const char *const DESC_MATERIAL = "material";
const char *const DESC_LIGHTING = "lighting";
const char *const DESC_IMGUI = "imgui";

class DescriptorManager {
public:
  std::pair<VkDescriptorPool *, VkDescriptorSetLayout *>
  operator[](const std::string &key);

  VkDescriptorPool *getPool(const std::string &key);
  VkDescriptorSetLayout *getSetLayout(const std::string &key);

  // Returns true if successful
  // Returns false if a key with that name already exists
  bool addPool(const std::string &key, VkDescriptorPool pool);
  bool addSetLayout(const std::string &key, VkDescriptorSetLayout setLayout);

  fstl::fixed_vector<VkDescriptorSetLayout> getDefaultSetLayouts();

  void init();
  void destroy();

protected:
  std::vector<std::pair<std::string, VkDescriptorPool>> m_pools;
  std::vector<std::pair<std::string, VkDescriptorSetLayout>> m_setLayouts;
};
} // namespace vkr
