#include "camera.hpp"
#include "engine.hpp"
#include <fstd/array.h>
#include <renderer/context.hpp>
#include <renderer/pipeline.hpp>
#include <renderer/util.hpp>
#include <renderer/window.hpp>

void eg_camera_init(eg_camera_t *camera) {
  camera->near = 0.001f;
  camera->far = 300.0f;

  camera->fov = to_radians(70.0f);

  camera->position = {0.0f, 0.0f, 0.0f};
  camera->rotation = {};

  {
    VkDescriptorSetLayout set_layouts[ARRAYSIZE(camera->descriptor_sets)];
    for (size_t i = 0; i < ARRAYSIZE(camera->descriptor_sets); i++) {
      set_layouts[i] = g_eng.set_layouts.camera;
    }

    VkDescriptorSetAllocateInfo alloc_info = {};
    alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    alloc_info.pNext = NULL;
    alloc_info.descriptorPool = g_ctx.descriptor_pool;
    alloc_info.descriptorSetCount = ARRAYSIZE(camera->descriptor_sets);
    alloc_info.pSetLayouts = set_layouts;

    VK_CHECK(vkAllocateDescriptorSets(
        g_ctx.device, &alloc_info, camera->descriptor_sets));
  }

  for (uint32_t i = 0; i < RE_MAX_FRAMES_IN_FLIGHT; i++) {
    re_buffer_init_uniform(
        &camera->uniform_buffers[i], sizeof(eg_camera_uniform_t));
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

void eg_camera_update(eg_camera_t *camera, struct re_window_t *window) {
  uint32_t i = window->current_frame;

  uint32_t width, height;
  re_window_get_size(window, &width, &height);

  camera->uniform.proj = mat4_perspective(
      camera->fov, (float)width / (float)height, camera->near, camera->far);

  // @note: See:
  // https://matthewwellings.com/blog/the-new-vulkan-coordinate-system/
  mat4_t correction = {{
      {1.0, 0.0, 0.0, 0.0},
      {0.0, -1.0, 0.0, 0.0},
      {0.0, 0.0, 0.5, 0.0},
      {0.0, 0.0, 0.5, 1.0},
  }};

  camera->uniform.proj = mat4_mul(camera->uniform.proj, correction);

  camera->uniform.pos = {
      camera->position.x, camera->position.y, camera->position.z, 1.0};

  memcpy(camera->mappings[i], &camera->uniform, sizeof(eg_camera_uniform_t));
}

void eg_camera_bind(
    eg_camera_t *camera,
    struct re_window_t *window,
    struct re_pipeline_t *pipeline,
    uint32_t set_index) {
  uint32_t i = window->current_frame;
  VkCommandBuffer command_buffer = re_window_get_current_command_buffer(window);

  vkCmdBindDescriptorSets(
      command_buffer,
      VK_PIPELINE_BIND_POINT_GRAPHICS,
      pipeline->layout,
      set_index,
      1,
      &camera->descriptor_sets[i],
      0,
      NULL);
}

void eg_camera_destroy(eg_camera_t *camera) {
  VK_CHECK(vkDeviceWaitIdle(g_ctx.device));

  vkFreeDescriptorSets(
      g_ctx.device,
      g_ctx.descriptor_pool,
      ARRAYSIZE(camera->descriptor_sets),
      camera->descriptor_sets);

  for (size_t i = 0; i < ARRAYSIZE(camera->uniform_buffers); i++) {
    re_buffer_unmap_memory(&camera->uniform_buffers[i]);
    re_buffer_destroy(&camera->uniform_buffers[i]);
  }
}
