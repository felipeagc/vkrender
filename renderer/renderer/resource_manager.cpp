#include "resource_manager.hpp"

#include "context.hpp"
#include "util.hpp"

using namespace renderer;

ResourceSetLayout::ResourceSetLayout(
    uint32_t maxSets,
    const fstl::fixed_vector<VkDescriptorSetLayoutBinding, 8> &bindings)
    : bindings(bindings), maxSets(maxSets) {
  VkDescriptorSetLayoutCreateInfo createInfo = {
      VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO, // sType
      nullptr,                                             // pNext
      0,                                                   // flags,
      static_cast<uint32_t>(this->bindings.size()),        // bindingCount
      this->bindings.data(),                               // pBindings
  };

  VK_CHECK(vkCreateDescriptorSetLayout(
      ctx().m_device, &createInfo, nullptr, &this->descriptorSetLayout));
}

ResourceSet ResourceSetLayout::allocate() {
  uint32_t found = -1;
  for (uint32_t i = 0; i < this->maxSets; i++) {
    if (this->bitset[i] == 0) {
      this->bitset[i] = 1;
      found = i;
      break;
    }
  }

  return ResourceSet(this->descriptorSets[found], found);
}

void ResourceSetLayout::free(ResourceSet &resourceSet) {
  if (this->bitset[resourceSet.allocation] == 1) {
    this->bitset[resourceSet.allocation] = 0;
    resourceSet.descriptorSet = VK_NULL_HANDLE;
  }
}

void ResourceSetLayout::destroy() {
  VK_CHECK(vkDeviceWaitIdle(ctx().m_device));

  if (this->descriptorSetLayout != VK_NULL_HANDLE) {
    vkDestroyDescriptorSetLayout(
        ctx().m_device, this->descriptorSetLayout, nullptr);
  }
}

ResourceSetProvider::ResourceSetProvider(
    const fstl::fixed_vector<ResourceSetLayout *> &setLayouts) {
  fstl::fixed_vector<VkDescriptorPoolSize> poolSizes;

  uint32_t maxSets = 0;

  for (auto &setLayout : setLayouts) {
    for (auto &binding : setLayout->bindings) {
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
            binding.descriptorCount * setLayout->maxSets,
        });
      } else {
        foundp->descriptorCount += binding.descriptorCount * setLayout->maxSets;
      }
    }

    maxSets += setLayout->maxSets;
  }

  // @todo: look at these sizes and see if they're correct
  VkDescriptorPoolCreateInfo createInfo = {
      VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO, // sType
      nullptr,                                       // pNext
      0,                                             // flags
      maxSets,                                       // maxSets
      static_cast<uint32_t>(poolSizes.size()),       // poolSizeCount
      poolSizes.data(),                              // pPoolSizes
  };

  VK_CHECK(vkCreateDescriptorPool(
      ctx().m_device, &createInfo, nullptr, &this->descriptorPool));

  fstl::fixed_vector<VkDescriptorSetLayout> descriptorSetLayouts;

  for (auto &setLayout : setLayouts) {
    descriptorSetLayouts.push_back(setLayout->descriptorSetLayout);
  }

  VkPushConstantRange pushConstantRange = {};
  pushConstantRange.stageFlags =
      VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
  pushConstantRange.offset = 0;
  pushConstantRange.size = 128;

  VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {
      VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
      nullptr,
      0,
      static_cast<uint32_t>(descriptorSetLayouts.size()), // setLayoutCount
      descriptorSetLayouts.data(),                        // pSetLayouts
      1,                  // pushConstantRangeCount
      &pushConstantRange, // pPushConstantRanges
  };

  VK_CHECK(vkCreatePipelineLayout(
      ctx().m_device,
      &pipelineLayoutCreateInfo,
      nullptr,
      &this->pipelineLayout));

  for (auto &setLayout : setLayouts) {
    setLayout->descriptorSets.resize(setLayout->maxSets);
    fstl::fixed_vector<VkDescriptorSetLayout> layouts;
    for (uint32_t i = 0; i < setLayout->maxSets; i++) {
      layouts.push_back(setLayout->descriptorSetLayout);
    }

    VkDescriptorSetAllocateInfo allocateInfo = {
        VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        nullptr,
        this->descriptorPool,
        static_cast<uint32_t>(layouts.size()),
        layouts.data(),
    };
    VK_CHECK(vkAllocateDescriptorSets(
        ctx().m_device, &allocateInfo, setLayout->descriptorSets.data()));
  }
}

void ResourceSetProvider::destroy() {
  VK_CHECK(vkDeviceWaitIdle(ctx().m_device));
  if (this->pipelineLayout != VK_NULL_HANDLE) {
    vkDestroyPipelineLayout(ctx().m_device, this->pipelineLayout, nullptr);
  }

  if (this->descriptorPool != VK_NULL_HANDLE) {
    vkDestroyDescriptorPool(ctx().m_device, this->descriptorPool, nullptr);
  }
}

void ResourceManager::initialize() {
  m_setLayouts.camera = ResourceSetLayout(
      10,
      {{
          0,                                 // binding
          VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, // descriptorType
          1,                                 // descriptorCount
          VK_SHADER_STAGE_VERTEX_BIT |
              VK_SHADER_STAGE_FRAGMENT_BIT, // stageFlags
          nullptr,                          // pImmutableSamplers
      }});

  m_setLayouts.mesh = ResourceSetLayout(
      100,
      {{
          0,                                 // binding
          VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, // descriptorType
          1,                                 // descriptorCount
          VK_SHADER_STAGE_VERTEX_BIT,        // stageFlags
          nullptr,                           // pImmutableSamplers
      }});

  m_setLayouts.model = ResourceSetLayout(
      100,
      {{
          0,                                 // binding
          VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, // descriptorType
          1,                                 // descriptorCount
          VK_SHADER_STAGE_VERTEX_BIT,        // stageFlags
          nullptr,                           // pImmutableSamplers
      }});

  m_setLayouts.material = ResourceSetLayout(
      100,
      {
          {
              0,                                         // binding
              VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, // descriptorType
              1,                                         // descriptorCount
              VK_SHADER_STAGE_FRAGMENT_BIT,              // stageFlags
              nullptr,                                   // pImmutableSamplers
          },
          {
              1,                                         // binding
              VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, // descriptorType
              1,                                         // descriptorCount
              VK_SHADER_STAGE_FRAGMENT_BIT,              // stageFlags
              nullptr,                                   // pImmutableSamplers
          },
          {
              2,                                         // binding
              VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, // descriptorType
              1,                                         // descriptorCount
              VK_SHADER_STAGE_FRAGMENT_BIT,              // stageFlags
              nullptr,                                   // pImmutableSamplers
          },
          {
              3,                                         // binding
              VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, // descriptorType
              1,                                         // descriptorCount
              VK_SHADER_STAGE_FRAGMENT_BIT,              // stageFlags
              nullptr,                                   // pImmutableSamplers
          },
          {
              4,                                         // binding
              VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, // descriptorType
              1,                                         // descriptorCount
              VK_SHADER_STAGE_FRAGMENT_BIT,              // stageFlags
              nullptr,                                   // pImmutableSamplers
          },
      });

  m_setLayouts.environment = ResourceSetLayout(
      10,
      {
          {
              0,                                 // binding
              VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, // descriptorType
              1,                                 // descriptorCount
              VK_SHADER_STAGE_FRAGMENT_BIT,      // stageFlags
              nullptr,                           // pImmutableSamplers
          },
          {
              1,                                         // binding
              VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, // descriptorType
              1,                                         // descriptorCount
              VK_SHADER_STAGE_FRAGMENT_BIT,              // stageFlags
              nullptr,                                   // pImmutableSamplers
          },
          {
              2,                                         // binding
              VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, // descriptorType
              1,                                         // descriptorCount
              VK_SHADER_STAGE_FRAGMENT_BIT,              // stageFlags
              nullptr,                                   // pImmutableSamplers
          },
          {
              3,                                         // binding
              VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, // descriptorType
              1,                                         // descriptorCount
              VK_SHADER_STAGE_FRAGMENT_BIT,              // stageFlags
              nullptr,                                   // pImmutableSamplers
          },
          {
              4,                                         // binding
              VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, // descriptorType
              1,                                         // descriptorCount
              VK_SHADER_STAGE_FRAGMENT_BIT,              // stageFlags
              nullptr,                                   // pImmutableSamplers
          },
      });

  m_setLayouts.fullscreen = ResourceSetLayout(
      20,
      {
          {
              0,                                         // binding
              VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, // descriptorType
              1,                                         // descriptorCount
              VK_SHADER_STAGE_FRAGMENT_BIT,              // stageFlags
              nullptr,                                   // pImmutableSamplers
          },
      });

  m_providers.standard = ResourceSetProvider{
      {&m_setLayouts.camera,
       &m_setLayouts.material,
       &m_setLayouts.mesh,
       &m_setLayouts.model,
       &m_setLayouts.environment},
  };

  m_providers.billboard = ResourceSetProvider{
      {&m_setLayouts.camera, &m_setLayouts.material, &m_setLayouts.model},
  };

  m_providers.skybox = ResourceSetProvider{
      {&m_setLayouts.camera, &m_setLayouts.environment},
  };

  m_providers.fullscreen = ResourceSetProvider{
      {&m_setLayouts.fullscreen},
  };

  m_providers.bakeCubemap = ResourceSetProvider{
      {&m_setLayouts.material},
  };
}

void ResourceManager::destroy() {
  m_providers.standard.destroy();
  m_providers.billboard.destroy();
  m_providers.skybox.destroy();
  m_providers.fullscreen.destroy();
  m_providers.bakeCubemap.destroy();

  m_setLayouts.camera.destroy();
  m_setLayouts.mesh.destroy();
  m_setLayouts.model.destroy();
  m_setLayouts.material.destroy();
  m_setLayouts.environment.destroy();
  m_setLayouts.fullscreen.destroy();
}
