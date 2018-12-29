#pragma once

#include <fstl/fixed_vector.hpp>
#include <unordered_map>
#include <utility>
#include <vulkan/vulkan.h>

namespace renderer {
const char *const DESC_IMGUI = "imgui";

class DescriptorManager {
public:
  VkDescriptorPool *getPool(const std::string &key);

  // Returns true if successful
  // Returns false if a key with that name already exists
  bool addPool(const std::string &key, VkDescriptorPool pool);

  void init();
  void destroy();

protected:
  std::unordered_map<std::string, VkDescriptorPool> m_pools;
};
} // namespace renderer
