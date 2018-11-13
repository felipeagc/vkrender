#pragma once

#include "aliases.hpp"
#include <fstl/fixed_vector.hpp>

namespace vkr {
class DescriptorSetLayout : public vk::DescriptorSetLayout {
public:
  DescriptorSetLayout(){};
  DescriptorSetLayout(
      const fstl::fixed_vector<DescriptorSetLayoutBinding> &bindings);
  ~DescriptorSetLayout(){};
  DescriptorSetLayout(const DescriptorSetLayout &) = default;
  DescriptorSetLayout &operator=(const DescriptorSetLayout &) = default;
  DescriptorSetLayout(DescriptorSetLayout &&) = default;
  DescriptorSetLayout &operator=(DescriptorSetLayout &&) = default;

  void destroy();
};

class DescriptorPool : public vk::DescriptorPool {
public:
  DescriptorPool(){};
  // Create a descriptor pool with sizes derived from the bindings and maxSets
  DescriptorPool(
      uint32_t maxSets,
      const fstl::fixed_vector<DescriptorSetLayoutBinding> &bindings);

  // Create a descriptor pool with manually specified poolSizes
  DescriptorPool(
      uint32_t maxSets,
      const fstl::fixed_vector<DescriptorPoolSize> &poolSizes);
  ~DescriptorPool(){};

  DescriptorPool(const DescriptorPool &) = default;
  DescriptorPool &operator=(const DescriptorPool &) = default;

  // Allocate many descriptor sets with one layout
  fstl::fixed_vector<DescriptorSet>
  allocateDescriptorSets(uint32_t setCount, DescriptorSetLayout &layout);

  // Allocate many descriptor sets with different layouts
  fstl::fixed_vector<DescriptorSet> allocateDescriptorSets(
      const fstl::fixed_vector<DescriptorSetLayout> &layouts);

  void destroy();
};

} // namespace vkr
