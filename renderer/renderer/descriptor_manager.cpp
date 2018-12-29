#include "descriptor_manager.hpp"
#include "context.hpp"
#include "util.hpp"
#include <vulkan/vulkan.h>

using namespace renderer;

VkDescriptorSetLayout createDescriptorSetLayout(
    uint32_t bindingCount, const VkDescriptorSetLayoutBinding *pBindings) {
  VkDescriptorSetLayout setLayout;

  VkDescriptorSetLayoutCreateInfo createInfo = {
      VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO, // sType
      nullptr,                                             // pNext
      0,                                                   // flags,
      bindingCount,                                        // bindingCount
      pBindings,                                           // pBindings
  };

  VK_CHECK(vkCreateDescriptorSetLayout(
      ctx().m_device, &createInfo, nullptr, &setLayout));

  return setLayout;
}

VkDescriptorPool createDescriptorPool(
    uint32_t maxSets,
    uint32_t bindingCount,
    const VkDescriptorSetLayoutBinding *pBindings) {
  fstl::fixed_vector<VkDescriptorPoolSize> poolSizes;

  for (uint32_t i = 0; i < bindingCount; i++) {
    auto binding = pBindings[i];
    VkDescriptorPoolSize *foundp = nullptr;
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

  VkDescriptorPool descriptorPool;

  VkDescriptorPoolCreateInfo createInfo = {
      VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,     // sType
      nullptr,                                           // pNext
      VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT, // flags
      maxSets,                                           // maxSets
      static_cast<uint32_t>(poolSizes.size()),           // poolSizeCount
      poolSizes.data(),                                  // pPoolSizes
  };

  VK_CHECK(vkCreateDescriptorPool(
      ctx().m_device, &createInfo, nullptr, &descriptorPool));

  return descriptorPool;
}

VkDescriptorPool *DescriptorManager::getPool(const std::string &key) {
  if (m_pools.find(key) != m_pools.end()) {
    return &m_pools[key];
  }

  return nullptr;
}

bool DescriptorManager::addPool(const std::string &key, VkDescriptorPool pool) {
  if (this->getPool(key)) {
    return false;
  }

  m_pools[key] = pool;
  return true;
}

void DescriptorManager::init() {
  // Imgui
  {
    VkDescriptorPoolSize imguiPoolSizes[] = {
        {VK_DESCRIPTOR_TYPE_SAMPLER, 1000},
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000},
        {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000},
        {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000},
        {VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000},
        {VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000},
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000},
        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000},
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000},
        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000},
        {VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000},
    };

    VkDescriptorPool imguiDescriptorPool;

    VkDescriptorPoolCreateInfo createInfo = {
        VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,           // sType
        nullptr,                                                 // pNext
        VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,       // flags
        1000 * static_cast<uint32_t>(ARRAYSIZE(imguiPoolSizes)), // maxSets
        static_cast<uint32_t>(ARRAYSIZE(imguiPoolSizes)), // poolSizeCount
        imguiPoolSizes,                                   // pPoolSizes
    };

    VK_CHECK(vkCreateDescriptorPool(
        ctx().m_device, &createInfo, nullptr, &imguiDescriptorPool));

    this->addPool(DESC_IMGUI, imguiDescriptorPool);
  }
}

void DescriptorManager::destroy() {
  VK_CHECK(vkDeviceWaitIdle(ctx().m_device));

  for (auto &p : m_pools) {
    vkDestroyDescriptorPool(ctx().m_device, p.second, nullptr);
  }
}
