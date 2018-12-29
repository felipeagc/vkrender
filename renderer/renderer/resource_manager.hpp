#pragma once

#include <bitset>
#include <fstl/fixed_vector.hpp>
#include <vulkan/vulkan.h>

namespace renderer {
const size_t GLOBAL_MAX_DESCRIPTOR_SETS = 1000;

struct ResourceSet {
  friend struct ResourceSetProvider;
  friend struct ResourceSetLayout;

  operator VkDescriptorSet() { return this->descriptorSet; }
  operator VkDescriptorSet*() { return &this->descriptorSet; }
  operator bool() { return this->descriptorSet != VK_NULL_HANDLE; }

  ResourceSet() {}

protected:
  VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
  uint32_t allocation = -1;

  ResourceSet(VkDescriptorSet descriptorSet, uint32_t allocation)
      : descriptorSet(descriptorSet), allocation(allocation) {}
};

struct ResourceSetLayout {
  friend class ResourceManager;
  friend struct ResourceSetProvider;

  operator VkDescriptorSetLayout() { return this->descriptorSetLayout; }

  ResourceSet allocate();
  void free(ResourceSet &resourceSet);

protected:
  ResourceSetLayout() {}
  ResourceSetLayout(
      uint32_t maxSets,
      const fstl::fixed_vector<VkDescriptorSetLayoutBinding, 8> &bindings);

  ResourceSetLayout(const ResourceSetLayout &) = delete;
  ResourceSetLayout &operator=(const ResourceSetLayout &) = delete;

  ResourceSetLayout(ResourceSetLayout &&old) {
    this->descriptorSetLayout = old.descriptorSetLayout;
    this->maxSets = old.maxSets;
    this->descriptorSets = old.descriptorSets;
    this->bitset = old.bitset;
    this->bindings = old.bindings;
    old.descriptorSetLayout = VK_NULL_HANDLE;
  }
  ResourceSetLayout &operator=(ResourceSetLayout &&old) {
    this->descriptorSetLayout = old.descriptorSetLayout;
    this->maxSets = old.maxSets;
    this->descriptorSets = old.descriptorSets;
    this->bitset = old.bitset;
    this->bindings = old.bindings;
    old.descriptorSetLayout = VK_NULL_HANDLE;
    return *this;
  }

  void destroy();

  VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
  fstl::fixed_vector<VkDescriptorSetLayoutBinding, 8> bindings;

  uint32_t maxSets = GLOBAL_MAX_DESCRIPTOR_SETS;

  // Preallocated descriptor sets
  fstl::fixed_vector<VkDescriptorSet> descriptorSets;
  std::bitset<GLOBAL_MAX_DESCRIPTOR_SETS> bitset;
};

struct ResourceSetProvider {
  friend class ResourceManager;

  VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
  VkDescriptorPool descriptorPool = VK_NULL_HANDLE;

protected:
  ResourceSetProvider(){};
  ResourceSetProvider(
      const fstl::fixed_vector<ResourceSetLayout *> &setLayouts);
  void destroy();
};

class ResourceManager {
public:
  ResourceManager(){};
  ~ResourceManager(){};

  // ResourceManager cannot be copied
  ResourceManager(const ResourceManager &) = delete;
  ResourceManager &operator=(const ResourceManager &) = delete;

  // ResourceManager cannot be moved
  ResourceManager(ResourceManager &&) = delete;
  ResourceManager &operator=(ResourceManager &&) = delete;

  // Initialize resource providers
  void initialize();

  void destroy();

  struct {
    ResourceSetProvider standard;
    ResourceSetProvider billboard;
    ResourceSetProvider skybox;
    ResourceSetProvider fullscreen;
    ResourceSetProvider bakeCubemap;
  } m_providers;

  struct {
    ResourceSetLayout camera;
    ResourceSetLayout mesh;
    ResourceSetLayout model;
    ResourceSetLayout material;
    ResourceSetLayout environment;
    ResourceSetLayout fullscreen;
  } m_setLayouts;
};
} // namespace renderer
