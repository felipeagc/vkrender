#include "engine.h"
#include <fstd_util.h>
#include <physfs.h>
#include <renderer/context.h>
#include <renderer/util.h>
#include <string.h>

eg_engine_t g_eng;

static inline void create_descriptor_set_layout(
    VkDescriptorSetLayoutBinding *bindings,
    uint32_t binding_count,
    VkDescriptorSetLayout *descriptor_set_layout) {
  VkDescriptorSetLayoutCreateInfo create_info = {0};
  create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  create_info.pNext = NULL;
  create_info.flags = 0;
  create_info.bindingCount = binding_count;
  create_info.pBindings = bindings;

  VK_CHECK(vkCreateDescriptorSetLayout(
      g_ctx.device, &create_info, NULL, descriptor_set_layout));
}

static inline void create_pipeline_layout(
    VkDescriptorSetLayout *set_layouts,
    uint32_t set_layout_count,
    VkPipelineLayout *pipeline_layout) {
  VkPushConstantRange push_constant_range = {
      .stageFlags = VK_SHADER_STAGE_ALL_GRAPHICS,
      .offset = 0,
      .size = 128,
  };

  VK_CHECK(vkCreatePipelineLayout(
      g_ctx.device,
      &(VkPipelineLayoutCreateInfo){
          .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
          .pNext = NULL,
          .flags = 0,
          .setLayoutCount = set_layout_count,
          .pSetLayouts = set_layouts,
          .pushConstantRangeCount = 1,
          .pPushConstantRanges = &push_constant_range,
      },
      NULL,
      pipeline_layout));
}

static inline void init_set_layouts() {
  // Camera
  {
    VkDescriptorSetLayoutBinding bindings[] = {{
        0,                                 // binding
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, // descriptorType
        1,                                 // descriptorCount
        VK_SHADER_STAGE_ALL_GRAPHICS,      // stageFlags
        NULL,                              // pImmutableSamplers
    }};

    create_descriptor_set_layout(
        bindings, ARRAY_SIZE(bindings), &g_eng.set_layouts.camera);
  }

  // Model
  {
    VkDescriptorSetLayoutBinding bindings[] = {{
        0,                                 // binding
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, // descriptorType
        1,                                 // descriptorCount
        VK_SHADER_STAGE_ALL_GRAPHICS,      // stageFlags
        NULL,                              // pImmutableSamplers
    }};

    create_descriptor_set_layout(
        bindings, ARRAY_SIZE(bindings), &g_eng.set_layouts.model);
  }

  // Environment
  {
    VkDescriptorSetLayoutBinding bindings[] = {
        {
            1,                                         // binding
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, // descriptorType
            1,                                         // descriptorCount
            VK_SHADER_STAGE_ALL_GRAPHICS,              // stageFlags
            NULL,                                      // pImmutableSamplers
        },
        {
            0,                                 // binding
            VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, // descriptorType
            1,                                 // descriptorCount
            VK_SHADER_STAGE_ALL_GRAPHICS,      // stageFlags
            NULL,                              // pImmutableSamplers
        },
        {
            2,                                         // binding
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, // descriptorType
            1,                                         // descriptorCount
            VK_SHADER_STAGE_ALL_GRAPHICS,              // stageFlags
            NULL,                                      // pImmutableSamplers
        },
        {
            3,                                         // binding
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, // descriptorType
            1,                                         // descriptorCount
            VK_SHADER_STAGE_ALL_GRAPHICS,              // stageFlags
            NULL,                                      // pImmutableSamplers
        },
        {
            4,                                         // binding
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, // descriptorType
            1,                                         // descriptorCount
            VK_SHADER_STAGE_ALL_GRAPHICS,              // stageFlags
            NULL,                                      // pImmutableSamplers
        }};

    create_descriptor_set_layout(
        bindings, ARRAY_SIZE(bindings), &g_eng.set_layouts.environment);
  }

  // Material
  {
    VkDescriptorSetLayoutBinding bindings[] = {
        {
            0,                                         // binding
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, // descriptorType
            1,                                         // descriptorCount
            VK_SHADER_STAGE_ALL_GRAPHICS,              // stageFlags
            NULL,                                      // pImmutableSamplers
        },
        {
            1,                                         // binding
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, // descriptorType
            1,                                         // descriptorCount
            VK_SHADER_STAGE_ALL_GRAPHICS,              // stageFlags
            NULL,                                      // pImmutableSamplers
        },
        {
            2,                                         // binding
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, // descriptorType
            1,                                         // descriptorCount
            VK_SHADER_STAGE_ALL_GRAPHICS,              // stageFlags
            NULL,                                      // pImmutableSamplers
        },
        {
            3,                                         // binding
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, // descriptorType
            1,                                         // descriptorCount
            VK_SHADER_STAGE_ALL_GRAPHICS,              // stageFlags
            NULL,                                      // pImmutableSamplers
        },
        {
            4,                                         // binding
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, // descriptorType
            1,                                         // descriptorCount
            VK_SHADER_STAGE_ALL_GRAPHICS,              // stageFlags
            NULL,                                      // pImmutableSamplers
        },
        {
            5,                                 // binding
            VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, // descriptorType
            1,                                 // descriptorCount
            VK_SHADER_STAGE_ALL_GRAPHICS,      // stageFlags
            NULL,                              // pImmutableSamplers
        },
    };

    create_descriptor_set_layout(
        bindings, ARRAY_SIZE(bindings), &g_eng.set_layouts.material);
  }
}

static inline void destroy_set_layouts() {
  vkDestroyDescriptorSetLayout(g_ctx.device, g_eng.set_layouts.camera, NULL);
  vkDestroyDescriptorSetLayout(g_ctx.device, g_eng.set_layouts.model, NULL);
  vkDestroyDescriptorSetLayout(
      g_ctx.device, g_eng.set_layouts.environment, NULL);
  vkDestroyDescriptorSetLayout(g_ctx.device, g_eng.set_layouts.material, NULL);
}

void eg_engine_init(const char *argv0) {
  PHYSFS_init(argv0);

  init_set_layouts();
}

void eg_engine_destroy() {
  destroy_set_layouts();

  PHYSFS_deinit();
}

