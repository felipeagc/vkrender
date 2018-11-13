#include "descriptor.hpp"
#include "context.hpp"

using namespace vkr;

DescriptorSetLayout::DescriptorSetLayout(
    const fstl::fixed_vector<vk::DescriptorSetLayoutBinding> &bindings)
    : vk::DescriptorSetLayout(Context::getDevice().createDescriptorSetLayout(
          {{}, static_cast<uint32_t>(bindings.size()), bindings.data()})) {}

void DescriptorSetLayout::destroy() {
  Context::getDevice().waitIdle();
  Context::getDevice().destroy(*this);
}

DescriptorPool::DescriptorPool(
    uint32_t maxSets,
    const fstl::fixed_vector<vk::DescriptorSetLayoutBinding> &bindings) {
  fstl::fixed_vector<vk::DescriptorPoolSize> poolSizes;

  for (auto &binding : bindings) {
    vk::DescriptorPoolSize *foundp = nullptr;
    for (auto &poolSize : poolSizes) {
      if (poolSize.type == binding.descriptorType) {
        foundp = &poolSize;
        break;
      }
    }

    if (foundp == nullptr) {
      poolSizes.push_back({
          binding.descriptorType,
          binding.descriptorCount,
      });
    } else {
      foundp->descriptorCount += binding.descriptorCount;
    }
  }

  for (auto &poolSize : poolSizes) {
    poolSize.descriptorCount *= maxSets;
  }

  vk::DescriptorPool descriptorPool =
      Context::getDevice().createDescriptorPool({
          vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet, // flags
          maxSets,                                              // maxSets
          static_cast<uint32_t>(poolSizes.size()),              // poolSizeCount
          poolSizes.data(),                                     // pPoolSizes
      });

  *this = static_cast<DescriptorPool &>(descriptorPool);
}

DescriptorPool::DescriptorPool(
    uint32_t maxSets,
    const fstl::fixed_vector<vk::DescriptorPoolSize> &poolSizes)
    : vk::DescriptorPool(Context::getDevice().createDescriptorPool(
          {{},
           maxSets,
           static_cast<uint32_t>(poolSizes.size()),
           poolSizes.data()})) {}

fstl::fixed_vector<DescriptorSet> DescriptorPool::allocateDescriptorSets(
    uint32_t setCount, DescriptorSetLayout &layout) {
  fstl::fixed_vector<DescriptorSetLayout> layouts(setCount, layout);
  return this->allocateDescriptorSets(layouts);
}

fstl::fixed_vector<DescriptorSet> DescriptorPool::allocateDescriptorSets(
    const fstl::fixed_vector<DescriptorSetLayout> &layouts) {
  auto sets = Context::getDevice().allocateDescriptorSets(
      {*this, static_cast<uint32_t>(layouts.size()), layouts.data()});

  fstl::fixed_vector<DescriptorSet> descriptorSets(sets.size());

  for (size_t i = 0; i < sets.size(); i++) {
    descriptorSets[i] = {sets[i]};
  }

  return descriptorSets;
}

void DescriptorPool::destroy() {
  Context::getDevice().waitIdle();
  Context::getDevice().destroy(*this);
}
