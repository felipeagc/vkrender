#include "resource_manager.hpp"

#include "context.hpp"
#include "util.hpp"
#include <stdlib.h>
#include <string.h>
#include <fstd/array.h>

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

  fstd_bitset_reset((uint8_t *)&layout->bitset, RE_GLOBAL_MAX_DESCRIPTOR_SETS);

  VkDescriptorSetLayoutCreateInfo createInfo = {
      VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO, // sType
      NULL,                                                // pNext
      0,                                                   // flags,
      layout->binding_count,                               // bindingCount
      layout->bindings,                                    // pBindings
  };

  VK_CHECK(vkCreateDescriptorSetLayout(
      g_ctx.device, &createInfo, NULL, &layout->descriptor_set_layout));
}

void re_resource_set_layout_destroy(re_resource_set_layout_t *layout) {
  VK_CHECK(vkDeviceWaitIdle(g_ctx.device));

  free(layout->descriptor_sets);
  free(layout->bindings);

  if (layout->descriptor_set_layout != VK_NULL_HANDLE) {
    vkDestroyDescriptorSetLayout(
        g_ctx.device, layout->descriptor_set_layout, NULL);
  }
}

re_resource_set_t re_allocate_resource_set(re_resource_set_layout_t *layout) {
  uint32_t found = -1;
  for (uint32_t i = 0; i < layout->max_sets; i++) {
    if (!fstd_bitset_at((uint8_t *)&layout->bitset, i)) {
      fstd_bitset_set((uint8_t *)&layout->bitset, i, true);
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
  if (fstd_bitset_at((uint8_t *)&layout->bitset, resource_set->allocation)) {
    fstd_bitset_set((uint8_t *)&layout->bitset, resource_set->allocation, false);
    resource_set->descriptor_set = VK_NULL_HANDLE;
  }
}

void re_resource_set_group_init(
    re_resource_set_group_t *group,
    re_resource_set_layout_t **set_layouts,
    uint32_t set_layout_count) {

  uint32_t pool_size_limit = 0;
  uint32_t pool_size_count = 0;

  for (uint32_t i = 0; i < set_layout_count; i++) {
    re_resource_set_layout_t *set_layout = set_layouts[i];
    for (uint32_t j = 0; j < set_layout->binding_count; j++) {
      pool_size_limit++;
    }
  }

  VkDescriptorPoolSize *pool_sizes = (VkDescriptorPoolSize *)malloc(
      sizeof(VkDescriptorPoolSize) * pool_size_limit);

  uint32_t pool_max_sets = 0;

  for (uint32_t i = 0; i < set_layout_count; i++) {
    re_resource_set_layout_t *set_layout = set_layouts[i];
    for (uint32_t j = 0; j < set_layout->binding_count; j++) {
      VkDescriptorSetLayoutBinding binding = set_layout->bindings[j];

      VkDescriptorPoolSize *foundp = NULL;
      for (uint32_t k = 0; k < pool_size_count; k++) {
        if (pool_sizes[k].type == binding.descriptorType) {
          foundp = &pool_sizes[k];
          break;
        }
      }

      if (foundp == NULL) {
        pool_sizes[pool_size_count++] = VkDescriptorPoolSize{
            binding.descriptorType,
            binding.descriptorCount * set_layout->max_sets,
        };
      } else {
        foundp->descriptorCount +=
            binding.descriptorCount * set_layout->max_sets;
      }
    }

    pool_max_sets += set_layout->max_sets;
  }

  // @TODO: look at these sizes and see if they're correct
  VkDescriptorPoolCreateInfo create_info = {
      VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO, // sType
      NULL,                                          // pNext
      0,                                             // flags
      pool_max_sets,                                 // maxSets
      pool_size_count,                               // poolSizeCount
      pool_sizes,                                    // pPoolSizes
  };

  VK_CHECK(vkCreateDescriptorPool(
      g_ctx.device, &create_info, NULL, &group->descriptor_pool));

  free(pool_sizes);

  VkDescriptorSetLayout *descriptor_set_layouts =
      (VkDescriptorSetLayout *)malloc(
          sizeof(VkDescriptorSetLayout) * set_layout_count);

  for (uint32_t i = 0; i < set_layout_count; i++) {
    descriptor_set_layouts[i] = set_layouts[i]->descriptor_set_layout;
  }

  VkPushConstantRange push_constant_range = {};
  push_constant_range.stageFlags =
      VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
  push_constant_range.offset = 0;
  push_constant_range.size = 128;

  VkPipelineLayoutCreateInfo pipeline_layout_create_info = {
      VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
      NULL,
      0,
      set_layout_count,       // setLayoutCount
      descriptor_set_layouts, // pSetLayouts
      1,                      // pushConstantRangeCount
      &push_constant_range,   // pPushConstantRanges
  };

  VK_CHECK(vkCreatePipelineLayout(
      g_ctx.device,
      &pipeline_layout_create_info,
      NULL,
      &group->pipeline_layout));

  free(descriptor_set_layouts);

  for (uint32_t i = 0; i < set_layout_count; i++) {
    re_resource_set_layout_t *set_layout = set_layouts[i];

    // If this set_layout is already initialized, skip it
    if (set_layout->descriptor_sets != NULL) {
      continue;
    }

    set_layout->descriptor_sets = (VkDescriptorSet *)malloc(
        sizeof(VkDescriptorSet) * set_layout->max_sets);

    VkDescriptorSetLayout *layouts = (VkDescriptorSetLayout *)malloc(
        sizeof(VkDescriptorSetLayout) * set_layout->max_sets);

    for (uint32_t i = 0; i < set_layout->max_sets; i++) {
      layouts[i] = set_layout->descriptor_set_layout;
    }

    VkDescriptorSetAllocateInfo allocateInfo = {
        VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        NULL,
        group->descriptor_pool,
        set_layout->max_sets,
        layouts,
    };

    VK_CHECK(vkAllocateDescriptorSets(
        g_ctx.device, &allocateInfo, set_layout->descriptor_sets));

    free(layouts);
  }
}

void re_resource_set_group_destroy(re_resource_set_group_t *group) {
  VK_CHECK(vkDeviceWaitIdle(g_ctx.device));
  if (group->pipeline_layout != VK_NULL_HANDLE) {
    vkDestroyPipelineLayout(g_ctx.device, group->pipeline_layout, NULL);
  }

  if (group->descriptor_pool != VK_NULL_HANDLE) {
    vkDestroyDescriptorPool(g_ctx.device, group->descriptor_pool, NULL);
  }
}

void re_resource_manager_init(re_resource_manager_t *manager) {
  VkDescriptorSetLayoutBinding camera_bindings[] = {{
      0,                                 // binding
      VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, // descriptorType
      1,                                 // descriptorCount
      VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, // stageFlags
      NULL, // pImmutableSamplers
  }};
  re_resource_set_layout_init(
      &manager->set_layouts.camera,
      10,
      camera_bindings,
      ARRAYSIZE(camera_bindings));

  VkDescriptorSetLayoutBinding model_bindings[] = {{
      0,                                 // binding
      VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, // descriptorType
      1,                                 // descriptorCount
      VK_SHADER_STAGE_VERTEX_BIT,        // stageFlags
      NULL,                              // pImmutableSamplers
  }};
  re_resource_set_layout_init(
      &manager->set_layouts.model,
      200,
      model_bindings,
      ARRAYSIZE(model_bindings));

  VkDescriptorSetLayoutBinding material_bindings[] = {
      {
          0,                                         // binding
          VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, // descriptorType
          1,                                         // descriptorCount
          VK_SHADER_STAGE_FRAGMENT_BIT,              // stageFlags
          NULL,                                      // pImmutableSamplers
      },
      {
          1,                                         // binding
          VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, // descriptorType
          1,                                         // descriptorCount
          VK_SHADER_STAGE_FRAGMENT_BIT,              // stageFlags
          NULL,                                      // pImmutableSamplers
      },
      {
          2,                                         // binding
          VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, // descriptorType
          1,                                         // descriptorCount
          VK_SHADER_STAGE_FRAGMENT_BIT,              // stageFlags
          NULL,                                      // pImmutableSamplers
      },
      {
          3,                                         // binding
          VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, // descriptorType
          1,                                         // descriptorCount
          VK_SHADER_STAGE_FRAGMENT_BIT,              // stageFlags
          NULL,                                      // pImmutableSamplers
      },
      {
          4,                                         // binding
          VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, // descriptorType
          1,                                         // descriptorCount
          VK_SHADER_STAGE_FRAGMENT_BIT,              // stageFlags
          NULL,                                      // pImmutableSamplers
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
          NULL,                              // pImmutableSamplers
      },
      {
          1,                                         // binding
          VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, // descriptorType
          1,                                         // descriptorCount
          VK_SHADER_STAGE_FRAGMENT_BIT,              // stageFlags
          NULL,                                      // pImmutableSamplers
      },
      {
          2,                                         // binding
          VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, // descriptorType
          1,                                         // descriptorCount
          VK_SHADER_STAGE_FRAGMENT_BIT,              // stageFlags
          NULL,                                      // pImmutableSamplers
      },
      {
          3,                                         // binding
          VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, // descriptorType
          1,                                         // descriptorCount
          VK_SHADER_STAGE_FRAGMENT_BIT,              // stageFlags
          NULL,                                      // pImmutableSamplers
      },
      {
          4,                                         // binding
          VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, // descriptorType
          1,                                         // descriptorCount
          VK_SHADER_STAGE_FRAGMENT_BIT,              // stageFlags
          NULL,                                      // pImmutableSamplers
      }};
  re_resource_set_layout_init(
      &manager->set_layouts.environment,
      10,
      environment_bindings,
      ARRAYSIZE(environment_bindings));

  VkDescriptorSetLayoutBinding single_texture_bindings[] = {{
      0,                                         // binding
      VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, // descriptorType
      1,                                         // descriptorCount
      VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, // stageFlags
      NULL, // pImmutableSamplers
  }};
  re_resource_set_layout_init(
      &manager->set_layouts.single_texture,
      20,
      single_texture_bindings,
      ARRAYSIZE(single_texture_bindings));

  re_resource_set_layout_t *pbr_set_layouts[] = {
      &manager->set_layouts.camera,
      &manager->set_layouts.material,
      &manager->set_layouts.model,
      &manager->set_layouts.model,
      &manager->set_layouts.environment};
  re_resource_set_group_init(
      &manager->groups.pbr,
      pbr_set_layouts,
      ARRAYSIZE(pbr_set_layouts));

  re_resource_set_layout_t *billboard_set_layouts[] = {
      &manager->set_layouts.camera,
      &manager->set_layouts.material,
      &manager->set_layouts.model,
  };
  re_resource_set_group_init(
      &manager->groups.billboard,
      billboard_set_layouts,
      ARRAYSIZE(billboard_set_layouts));

  re_resource_set_layout_t *wireframe_set_layouts[] = {
      &manager->set_layouts.camera,
      &manager->set_layouts.model,
  };
  re_resource_set_group_init(
      &manager->groups.wireframe,
      wireframe_set_layouts,
      ARRAYSIZE(wireframe_set_layouts));

  re_resource_set_layout_t *skybox_set_layouts[] = {
      &manager->set_layouts.camera,
      &manager->set_layouts.environment,
  };
  re_resource_set_group_init(
      &manager->groups.skybox,
      skybox_set_layouts,
      ARRAYSIZE(skybox_set_layouts));

  re_resource_set_layout_t *fullscreen_set_layouts[] = {
      &manager->set_layouts.single_texture,
  };
  re_resource_set_group_init(
      &manager->groups.fullscreen,
      fullscreen_set_layouts,
      ARRAYSIZE(fullscreen_set_layouts));

  re_resource_set_layout_t *bake_cubemap_set_layouts[] = {
      &manager->set_layouts.material,
  };
  re_resource_set_group_init(
      &manager->groups.bake_cubemap,
      bake_cubemap_set_layouts,
      ARRAYSIZE(bake_cubemap_set_layouts));
}

void re_resource_manager_destroy(re_resource_manager_t *manager) {
  re_resource_set_group_destroy(&manager->groups.pbr);
  re_resource_set_group_destroy(&manager->groups.billboard);
  re_resource_set_group_destroy(&manager->groups.wireframe);
  re_resource_set_group_destroy(&manager->groups.skybox);
  re_resource_set_group_destroy(&manager->groups.fullscreen);
  re_resource_set_group_destroy(&manager->groups.bake_cubemap);

  re_resource_set_layout_destroy(&manager->set_layouts.camera);
  re_resource_set_layout_destroy(&manager->set_layouts.model);
  re_resource_set_layout_destroy(&manager->set_layouts.material);
  re_resource_set_layout_destroy(&manager->set_layouts.environment);
  re_resource_set_layout_destroy(&manager->set_layouts.single_texture);
}
