#include "camera.h"
#include "engine.h"
#include <fstd_util.h>
#include <renderer/context.h>
#include <renderer/pipeline.h>
#include <renderer/util.h>
#include <renderer/window.h>
#include <string.h>

void eg_camera_init(eg_camera_t *camera) {
  camera->near_clip = 0.001f;
  camera->far_clip = 300.0f;

  camera->fov = to_radians(70.0f);

  camera->position = (vec3_t){0.0f, 0.0f, 0.0f};
  camera->rotation = (quat_t){0};

  {
    VkDescriptorSetLayout set_layouts[ARRAY_SIZE(camera->descriptor_sets)];
    for (size_t i = 0; i < ARRAY_SIZE(camera->descriptor_sets); i++) {
      set_layouts[i] = g_eng.set_layouts.camera;
    }

    VkDescriptorSetAllocateInfo alloc_info = {0};
    alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    alloc_info.pNext = NULL;
    alloc_info.descriptorPool = g_ctx.descriptor_pool;
    alloc_info.descriptorSetCount = ARRAY_SIZE(camera->descriptor_sets);
    alloc_info.pSetLayouts = set_layouts;

    VK_CHECK(vkAllocateDescriptorSets(
        g_ctx.device, &alloc_info, camera->descriptor_sets));
  }

  for (uint32_t i = 0; i < RE_MAX_FRAMES_IN_FLIGHT; i++) {
    re_buffer_init(
        &camera->uniform_buffers[i],
        &(re_buffer_options_t){
            .type = RE_BUFFER_TYPE_UNIFORM,
            .size = sizeof(eg_camera_uniform_t),
        });
    re_buffer_map_memory(&camera->uniform_buffers[i], &camera->mappings[i]);

    VkDescriptorBufferInfo buffer_info = {
        camera->uniform_buffers[i].buffer,
        0,
        sizeof(eg_camera_uniform_t),
    };

    VkWriteDescriptorSet descriptor_write = {
        VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        NULL,
        camera->descriptor_sets[i],        // dstSet
        0,                                 // dstBinding
        0,                                 // dstArrayElement
        1,                                 // descriptorCount
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, // descriptorType
        NULL,                              // pImageInfo
        &buffer_info,                      // pBufferInfo
        NULL,                              // pTexelBufferView
    };

    vkUpdateDescriptorSets(g_ctx.device, 1, &descriptor_write, 0, NULL);
  }
}

void eg_camera_update(
    eg_camera_t *camera,
    const eg_cmd_info_t *cmd_info,
    float width,
    float height) {
  camera->uniform.proj = mat4_perspective(
      camera->fov, width / height, camera->near_clip, camera->far_clip);

  // @note: See:
  // https://matthewwellings.com/blog/the-new-vulkan-coordinate-system/
  mat4_t correction = {{
      1.0,
      0.0,
      0.0,
      0.0,
      0.0,
      -1.0,
      0.0,
      0.0,
      0.0,
      0.0,
      0.5,
      0.0,
      0.0,
      0.0,
      0.5,
      1.0,
  }};

  camera->uniform.proj = mat4_mul(camera->uniform.proj, correction);

  camera->uniform.pos =
      (vec4_t){camera->position.x, camera->position.y, camera->position.z, 1.0};

  memcpy(
      camera->mappings[cmd_info->frame_index],
      &camera->uniform,
      sizeof(eg_camera_uniform_t));
}

void eg_camera_bind(
    eg_camera_t *camera,
    const eg_cmd_info_t *cmd_info,
    struct re_pipeline_t *pipeline,
    uint32_t set_index) {
  vkCmdBindDescriptorSets(
      cmd_info->cmd_buffer,
      VK_PIPELINE_BIND_POINT_GRAPHICS,
      pipeline->layout,
      set_index,
      1,
      &camera->descriptor_sets[cmd_info->frame_index],
      0,
      NULL);
}

void eg_camera_destroy(eg_camera_t *camera) {
  VK_CHECK(vkDeviceWaitIdle(g_ctx.device));

  vkFreeDescriptorSets(
      g_ctx.device,
      g_ctx.descriptor_pool,
      ARRAY_SIZE(camera->descriptor_sets),
      camera->descriptor_sets);

  for (size_t i = 0; i < ARRAY_SIZE(camera->uniform_buffers); i++) {
    re_buffer_unmap_memory(&camera->uniform_buffers[i]);
    re_buffer_destroy(&camera->uniform_buffers[i]);
  }
}
