#include "descriptor_manager.hpp"
#include "context.hpp"
#include "util.hpp"
#include <vulkan/vulkan.h>

using namespace vkr;

const uint32_t CAMERA_MAX_SETS = 20;
const uint32_t MESH_MAX_SETS = 500;
const uint32_t MATERIAL_MAX_SETS = 50;
const uint32_t LIGHTING_MAX_SETS = 50;

const VkDescriptorSetLayoutBinding CAMERA_BINDINGS[] = {{
    0,                                 // binding
    VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, // descriptorType
    1,                                 // descriptorCount
    VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, // stageFlags
    nullptr, // pImmutableSamplers
}};

const VkDescriptorSetLayoutBinding MESH_BINDINGS[] = {{
    0,                                 // binding
    VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, // descriptorType
    1,                                 // descriptorCount
    VK_SHADER_STAGE_VERTEX_BIT,        // stageFlags
    nullptr,                           // pImmutableSamplers
}};

const VkDescriptorSetLayoutBinding MATERIAL_BINDINGS[] = {
    {
        0,                                         // binding
        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, // descriptorType
        1,                                         // descriptorCount
        VK_SHADER_STAGE_FRAGMENT_BIT,              // stageFlags
        nullptr,                                   // pImmutableSamplers
    },
    {
        1,                                 // binding
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, // descriptorType
        1,                                 // descriptorCount
        VK_SHADER_STAGE_FRAGMENT_BIT,      // stageFlags
        nullptr,                           // pImmutableSamplers
    },
};

const VkDescriptorSetLayoutBinding LIGHTING_BINDINGS[] = {
    {
        0,                                 // binding
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, // descriptorType
        1,                                 // descriptorCount
        VK_SHADER_STAGE_FRAGMENT_BIT,      // stageFlags
        nullptr,                           // pImmutableSamplers
    },
};

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
      ctx::device, &createInfo, nullptr, &setLayout));

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
      ctx::device, &createInfo, nullptr, &descriptorPool));

  return descriptorPool;
}

std::pair<VkDescriptorPool *, VkDescriptorSetLayout *> DescriptorManager::
operator[](const std::string &key) {
  return {this->getPool(key), this->getSetLayout(key)};
}

VkDescriptorPool *DescriptorManager::getPool(const std::string &key) {
  for (auto &p : this->pools_) {
    if (p.first == key) {
      return &p.second;
    }
  }

  return nullptr;
}

VkDescriptorSetLayout *DescriptorManager::getSetLayout(const std::string &key) {
  for (auto &p : this->setLayouts_) {
    if (p.first == key) {
      return &p.second;
    }
  }

  return nullptr;
}

bool DescriptorManager::addPool(const std::string &key, VkDescriptorPool pool) {
  if (this->getPool(key)) {
    return false;
  }

  pools_.push_back({key, pool});
  return true;
}

bool DescriptorManager::addSetLayout(
    const std::string &key, VkDescriptorSetLayout setLayout) {
  if (this->getSetLayout(key)) {
    return false;
  }

  setLayouts_.push_back({key, setLayout});
  return true;
}

fstl::fixed_vector<VkDescriptorSetLayout>
DescriptorManager::getDefaultSetLayouts() {
  return fstl::fixed_vector<VkDescriptorSetLayout>{
      *this->getSetLayout(vkr::DESC_CAMERA),
      *this->getSetLayout(vkr::DESC_MATERIAL),
      *this->getSetLayout(vkr::DESC_MESH),
      *this->getSetLayout(vkr::DESC_LIGHTING),
  };
}

void DescriptorManager::init() {
  this->addPool(
      DESC_CAMERA,
      createDescriptorPool(
          CAMERA_MAX_SETS, ARRAYSIZE(CAMERA_BINDINGS), CAMERA_BINDINGS));
  this->addSetLayout(
      DESC_CAMERA,
      createDescriptorSetLayout(ARRAYSIZE(CAMERA_BINDINGS), CAMERA_BINDINGS));

  this->addPool(
      DESC_MATERIAL,
      createDescriptorPool(
          MATERIAL_MAX_SETS, ARRAYSIZE(MATERIAL_BINDINGS), MATERIAL_BINDINGS));
  this->addSetLayout(
      DESC_MATERIAL,
      createDescriptorSetLayout(
          ARRAYSIZE(MATERIAL_BINDINGS), MATERIAL_BINDINGS));

  this->addPool(
      DESC_MESH,
      createDescriptorPool(
          MESH_MAX_SETS, ARRAYSIZE(MESH_BINDINGS), MESH_BINDINGS));
  this->addSetLayout(
      DESC_MESH,
      createDescriptorSetLayout(ARRAYSIZE(MESH_BINDINGS), MESH_BINDINGS));

  this->addPool(
      DESC_LIGHTING,
      createDescriptorPool(
          LIGHTING_MAX_SETS, ARRAYSIZE(LIGHTING_BINDINGS), LIGHTING_BINDINGS));
  this->addSetLayout(
      DESC_LIGHTING,
      createDescriptorSetLayout(
          ARRAYSIZE(LIGHTING_BINDINGS), LIGHTING_BINDINGS));

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
        ctx::device, &createInfo, nullptr, &imguiDescriptorPool));

    this->addPool(DESC_IMGUI, imguiDescriptorPool);
  }
}

void DescriptorManager::destroy() {
  for (auto &p : this->pools_) {
    vkDestroyDescriptorPool(ctx::device, p.second, nullptr);
  }

  for (auto &p : this->setLayouts_) {
    vkDestroyDescriptorSetLayout(ctx::device, p.second, nullptr);
  }
}
