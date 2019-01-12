#include "resource_manager.hpp"

#include "context.hpp"
#include "util.hpp"
#include <stdlib.h>

using namespace renderer;

void re_resource_set_layout_init(
    re_resource_set_layout_t *layout,
    uint32_t max_sets,
    VkDescriptorSetLayoutBinding *bindings,
    uint32_t binding_count) {
  layout->descriptor_sets = NULL;
  layout->max_sets = 0;
  layout->max_sets = max_sets;
  layout->binding_count = binding_count;
  layout->bindings = (VkDescriptorSetLayoutBinding *)malloc(
      sizeof(VkDescriptorSetLayoutBinding) * binding_count);
  memcpy(
      layout->bindings,
      bindings,
      sizeof(VkDescriptorSetLayoutBinding) * binding_count);

  VkDescriptorSetLayoutCreateInfo createInfo = {
      VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO, // sType
      nullptr,                                             // pNext
      0,                                                   // flags,
      layout->binding_count,                               // bindingCount
      layout->bindings,                                    // pBindings
  };

  VK_CHECK(vkCreateDescriptorSetLayout(
      ctx().m_device, &createInfo, nullptr, &layout->descriptor_set_layout));
}

re_resource_set_t re_allocate_resource_set(re_resource_set_layout_t *layout) {
  uint32_t found = -1;
  for (uint32_t i = 0; i < layout->max_sets; i++) {
    if (layout->bitset[i] == 0) {
      layout->bitset[i] = 1;
      found = i;
      break;
    }
  }

  re_resource_set_t resource_set;
  resource_set.descriptor_set = layout->descriptor_sets[found];
  resource_set.allocation = found;
  return resource_set;
}

void re_free_resource_set(
    re_resource_set_layout_t *layout, re_resource_set_t *resource_set) {
  if (layout->bitset[resource_set->allocation] == 1) {
    layout->bitset[resource_set->allocation] = 0;
    resource_set->descriptor_set = VK_NULL_HANDLE;
  }
}

void re_resource_set_layout_destroy(re_resource_set_layout_t *layout) {
  VK_CHECK(vkDeviceWaitIdle(ctx().m_device));

  if (layout->descriptor_sets != NULL) {
    free(layout->descriptor_sets);
  }

  if (layout->descriptor_set_layout != VK_NULL_HANDLE) {
    vkDestroyDescriptorSetLayout(
        ctx().m_device, layout->descriptor_set_layout, nullptr);
  }
}

void re_resource_set_provider_init(
    re_resource_set_provider_t *provider,
    re_resource_set_layout_t **set_layouts,
    uint32_t set_layout_count) {
  ftl::small_vector<VkDescriptorPoolSize> poolSizes;

  uint32_t pool_max_sets = 0;

  for (uint32_t i = 0; i < set_layout_count; i++) {
    auto &set_layout = set_layouts[i];
    for (uint32_t j = 0; j < set_layout->binding_count; j++) {
      auto &binding = set_layout->bindings[j];

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
            binding.descriptorCount * set_layout->max_sets,
        });
      } else {
        foundp->descriptorCount +=
            binding.descriptorCount * set_layout->max_sets;
      }
    }

    pool_max_sets += set_layout->max_sets;
  }

  // @todo: look at these sizes and see if they're correct
  VkDescriptorPoolCreateInfo createInfo = {
      VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO, // sType
      nullptr,                                       // pNext
      0,                                             // flags
      pool_max_sets,                                 // maxSets
      static_cast<uint32_t>(poolSizes.size()),       // poolSizeCount
      poolSizes.data(),                              // pPoolSizes
  };

  VK_CHECK(vkCreateDescriptorPool(
      ctx().m_device, &createInfo, nullptr, &provider->descriptor_pool));

  ftl::small_vector<VkDescriptorSetLayout> descriptorSetLayouts;

  for (uint32_t i = 0; i < set_layout_count; i++) {
    descriptorSetLayouts.push_back(set_layouts[i]->descriptor_set_layout);
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
      &provider->pipeline_layout));

  for (uint32_t i = 0; i < set_layout_count; i++) {
    auto &set_layout = set_layouts[i];

    set_layout->descriptor_sets = (VkDescriptorSet *)malloc(
        sizeof(VkDescriptorSet) * set_layout->max_sets);

    VkDescriptorSetLayout *layouts = (VkDescriptorSetLayout *)malloc(
        sizeof(VkDescriptorSetLayout) * set_layout->max_sets);

    for (uint32_t i = 0; i < set_layout->max_sets; i++) {
      layouts[i] = set_layout->descriptor_set_layout;
    }

    VkDescriptorSetAllocateInfo allocateInfo = {
        VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        nullptr,
        provider->descriptor_pool,
        set_layout->max_sets,
        layouts,
    };

    VK_CHECK(vkAllocateDescriptorSets(
        ctx().m_device, &allocateInfo, set_layout->descriptor_sets));

    free(layouts);
  }
}

void re_resource_set_provider_destroy(re_resource_set_provider_t *provider) {
  VK_CHECK(vkDeviceWaitIdle(ctx().m_device));
  if (provider->pipeline_layout != VK_NULL_HANDLE) {
    vkDestroyPipelineLayout(ctx().m_device, provider->pipeline_layout, nullptr);
  }

  if (provider->descriptor_pool != VK_NULL_HANDLE) {
    vkDestroyDescriptorPool(ctx().m_device, provider->descriptor_pool, nullptr);
  }
}

void re_resource_manager_init(re_resource_manager_t *manager) {
  VkDescriptorSetLayoutBinding camera_bindings[] = {{
      0,                                 // binding
      VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, // descriptorType
      1,                                 // descriptorCount
      VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, // stageFlags
      nullptr, // pImmutableSamplers
  }};
  re_resource_set_layout_init(
      &manager->set_layouts.camera,
      10,
      camera_bindings,
      ARRAYSIZE(camera_bindings));

  VkDescriptorSetLayoutBinding mesh_bindings[] = {{
      0,                                 // binding
      VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, // descriptorType
      1,                                 // descriptorCount
      VK_SHADER_STAGE_VERTEX_BIT,        // stageFlags
      nullptr,                           // pImmutableSamplers
  }};
  re_resource_set_layout_init(
      &manager->set_layouts.mesh, 100, mesh_bindings, ARRAYSIZE(mesh_bindings));

  VkDescriptorSetLayoutBinding model_bindings[] = {{
      0,                                 // binding
      VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, // descriptorType
      1,                                 // descriptorCount
      VK_SHADER_STAGE_VERTEX_BIT,        // stageFlags
      nullptr,                           // pImmutableSamplers
  }};
  re_resource_set_layout_init(
      &manager->set_layouts.model,
      100,
      model_bindings,
      ARRAYSIZE(model_bindings));

  VkDescriptorSetLayoutBinding material_bindings[] = {
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
      }};
  re_resource_set_layout_init(
      &manager->set_layouts.material,
      100,
      material_bindings,
      ARRAYSIZE(material_bindings));

  VkDescriptorSetLayoutBinding environment_bindings[] = {
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
      }};
  re_resource_set_layout_init(
      &manager->set_layouts.environment,
      10,
      environment_bindings,
      ARRAYSIZE(environment_bindings));

  VkDescriptorSetLayoutBinding fullscreen_bindings[] = {{
      0,                                         // binding
      VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, // descriptorType
      1,                                         // descriptorCount
      VK_SHADER_STAGE_FRAGMENT_BIT,              // stageFlags
      nullptr,                                   // pImmutableSamplers
  }};
  re_resource_set_layout_init(
      &manager->set_layouts.fullscreen,
      20,
      fullscreen_bindings,
      ARRAYSIZE(fullscreen_bindings));

  re_resource_set_layout_t *standard_set_layouts[] = {
      &manager->set_layouts.camera,
      &manager->set_layouts.material,
      &manager->set_layouts.mesh,
      &manager->set_layouts.model,
      &manager->set_layouts.environment};
  re_resource_set_provider_init(
      &manager->providers.standard,
      standard_set_layouts,
      ARRAYSIZE(standard_set_layouts));

  re_resource_set_layout_t *billboard_set_layouts[] = {
      &manager->set_layouts.camera,
      &manager->set_layouts.material,
      &manager->set_layouts.model,
  };
  re_resource_set_provider_init(
      &manager->providers.billboard,
      billboard_set_layouts,
      ARRAYSIZE(billboard_set_layouts));

  re_resource_set_layout_t *wireframe_set_layouts[] = {
      &manager->set_layouts.camera,
      &manager->set_layouts.model,
  };
  re_resource_set_provider_init(
      &manager->providers.wireframe,
      wireframe_set_layouts,
      ARRAYSIZE(wireframe_set_layouts));

  re_resource_set_layout_t *skybox_set_layouts[] = {
      &manager->set_layouts.camera,
      &manager->set_layouts.environment,
  };
  re_resource_set_provider_init(
      &manager->providers.skybox,
      skybox_set_layouts,
      ARRAYSIZE(skybox_set_layouts));

  re_resource_set_layout_t *fullscreen_set_layouts[] = {
      &manager->set_layouts.fullscreen,
  };
  re_resource_set_provider_init(
      &manager->providers.fullscreen,
      fullscreen_set_layouts,
      ARRAYSIZE(fullscreen_set_layouts));

  re_resource_set_layout_t *bake_cubemap_set_layouts[] = {
      &manager->set_layouts.material,
  };
  re_resource_set_provider_init(
      &manager->providers.bake_cubemap,
      bake_cubemap_set_layouts,
      ARRAYSIZE(bake_cubemap_set_layouts));
}

void re_resource_manager_destroy(re_resource_manager_t *manager) {
  re_resource_set_provider_destroy(&manager->providers.standard);
  re_resource_set_provider_destroy(&manager->providers.billboard);
  re_resource_set_provider_destroy(&manager->providers.wireframe);
  re_resource_set_provider_destroy(&manager->providers.skybox);
  re_resource_set_provider_destroy(&manager->providers.fullscreen);
  re_resource_set_provider_destroy(&manager->providers.bake_cubemap);

  re_resource_set_layout_destroy(&manager->set_layouts.camera);
  re_resource_set_layout_destroy(&manager->set_layouts.mesh);
  re_resource_set_layout_destroy(&manager->set_layouts.model);
  re_resource_set_layout_destroy(&manager->set_layouts.material);
  re_resource_set_layout_destroy(&manager->set_layouts.environment);
  re_resource_set_layout_destroy(&manager->set_layouts.fullscreen);
}
