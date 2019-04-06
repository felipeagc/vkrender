#include "engine.h"
#include <fstd_util.h>
#include <renderer/context.h>
#include <renderer/util.h>

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
  VkPushConstantRange push_constant_range = {0};
  push_constant_range.stageFlags = VK_SHADER_STAGE_ALL_GRAPHICS;
  push_constant_range.offset = 0;
  push_constant_range.size = 128;

  VkPipelineLayoutCreateInfo create_info;
  create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  create_info.pNext = NULL;
  create_info.flags = 0;
  create_info.setLayoutCount = set_layout_count;
  create_info.pSetLayouts = set_layouts;
  create_info.pushConstantRangeCount = 1;
  create_info.pPushConstantRanges = &push_constant_range;

  VK_CHECK(vkCreatePipelineLayout(
      g_ctx.device, &create_info, NULL, pipeline_layout));
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
        bindings, ARRAYSIZE(bindings), &g_eng.set_layouts.camera);
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
        bindings, ARRAYSIZE(bindings), &g_eng.set_layouts.model);
  }

  // Environment
  {
    VkDescriptorSetLayoutBinding bindings[] = {
        {
            0,                                 // binding
            VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, // descriptorType
            1,                                 // descriptorCount
            VK_SHADER_STAGE_ALL_GRAPHICS,      // stageFlags
            NULL,                              // pImmutableSamplers
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
        }};

    create_descriptor_set_layout(
        bindings, ARRAYSIZE(bindings), &g_eng.set_layouts.environment);
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
        }};

    create_descriptor_set_layout(
        bindings, ARRAYSIZE(bindings), &g_eng.set_layouts.material);
  }
}

static inline void destroy_set_layouts() {
  vkDestroyDescriptorSetLayout(g_ctx.device, g_eng.set_layouts.camera, NULL);
  vkDestroyDescriptorSetLayout(g_ctx.device, g_eng.set_layouts.model, NULL);
  vkDestroyDescriptorSetLayout(
      g_ctx.device, g_eng.set_layouts.environment, NULL);
  vkDestroyDescriptorSetLayout(g_ctx.device, g_eng.set_layouts.material, NULL);
}

static inline void init_pipeline_layouts() {
  // PBR
  {
    VkDescriptorSetLayout set_layouts[] = {
        g_eng.set_layouts.camera,
        g_eng.set_layouts.environment,
        g_eng.set_layouts.model,
        g_eng.set_layouts.model,
        g_eng.set_layouts.material,
    };

    create_pipeline_layout(
        set_layouts, ARRAYSIZE(set_layouts), &g_eng.pipeline_layouts.pbr);
  }

  // Billboard
  {
    VkDescriptorSetLayout set_layouts[] = {
        g_eng.set_layouts.camera,
        g_eng.set_layouts.material,
    };

    create_pipeline_layout(
        set_layouts, ARRAYSIZE(set_layouts), &g_eng.pipeline_layouts.billboard);
  }

  // Wireframe
  {
    VkDescriptorSetLayout set_layouts[] = {
        g_eng.set_layouts.camera,
        g_eng.set_layouts.model,
    };

    create_pipeline_layout(
        set_layouts, ARRAYSIZE(set_layouts), &g_eng.pipeline_layouts.wireframe);
  }

  // Skybox
  {
    VkDescriptorSetLayout set_layouts[] = {
        g_eng.set_layouts.camera,
        g_eng.set_layouts.environment,
    };

    create_pipeline_layout(
        set_layouts, ARRAYSIZE(set_layouts), &g_eng.pipeline_layouts.skybox);
  }

  // Fullscreen
  {
    VkDescriptorSetLayout set_layouts[] = {
        g_ctx.canvas_descriptor_set_layout,
    };

    create_pipeline_layout(
        set_layouts,
        ARRAYSIZE(set_layouts),
        &g_eng.pipeline_layouts.fullscreen);
  }
}

static inline void destroy_pipeline_layouts() {
  vkDestroyPipelineLayout(g_ctx.device, g_eng.pipeline_layouts.pbr, NULL);
  vkDestroyPipelineLayout(g_ctx.device, g_eng.pipeline_layouts.billboard, NULL);
  vkDestroyPipelineLayout(g_ctx.device, g_eng.pipeline_layouts.wireframe, NULL);
  vkDestroyPipelineLayout(g_ctx.device, g_eng.pipeline_layouts.skybox, NULL);
  vkDestroyPipelineLayout(
      g_ctx.device, g_eng.pipeline_layouts.fullscreen, NULL);
}

void eg_engine_init() {
  init_set_layouts();
  init_pipeline_layouts();
}

void eg_engine_destroy() {
  destroy_pipeline_layouts();
  destroy_set_layouts();
}
