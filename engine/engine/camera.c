#include "camera.h"
#include "pipelines.h"
#include <float.h>
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

  VkDescriptorSetLayout set_layout =
      g_default_pipeline_layouts.pbr.set_layouts[0];
  VkDescriptorUpdateTemplate update_template =
      g_default_pipeline_layouts.pbr.update_templates[0];

  {
    VkDescriptorSetLayout set_layouts[ARRAY_SIZE(camera->descriptor_sets)];
    for (size_t i = 0; i < ARRAY_SIZE(camera->descriptor_sets); i++) {
      set_layouts[i] = set_layout;
    }

    VK_CHECK(vkAllocateDescriptorSets(
        g_ctx.device,
        &(VkDescriptorSetAllocateInfo){
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
            .descriptorPool = g_ctx.descriptor_pool,
            .descriptorSetCount = ARRAY_SIZE(camera->descriptor_sets),
            .pSetLayouts = set_layouts,
        },
        camera->descriptor_sets));
  }

  for (uint32_t i = 0; i < RE_MAX_FRAMES_IN_FLIGHT; i++) {
    re_buffer_init(
        &camera->uniform_buffers[i],
        &(re_buffer_options_t){
            .type = RE_BUFFER_TYPE_UNIFORM,
            .size = sizeof(eg_camera_uniform_t),
        });
    re_buffer_map_memory(&camera->uniform_buffers[i], &camera->mappings[i]);

    vkUpdateDescriptorSetWithTemplate(
        g_ctx.device,
        camera->descriptor_sets[i],
        update_template,
        (re_descriptor_info_t[]){
            {.buffer = {.buffer = camera->uniform_buffers[i].buffer,
                        .offset = 0,
                        .range = sizeof(eg_camera_uniform_t)}}});
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
      {1.0, 0.0, 0.0, 0.0},
      {0.0, -1.0, 0.0, 0.0},
      {0.0, 0.0, 0.5, 0.0},
      {0.0, 0.0, 0.5, 1.0},
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
      pipeline->layout.layout,
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

vec3_t eg_camera_ndc_to_world(eg_camera_t *camera, vec3_t ndc) {
  mat4_t inv_view = mat4_inverse(camera->uniform.view);
  mat4_t inv_proj = mat4_inverse(camera->uniform.proj);

  vec4_t world = mat4_mulv(inv_proj, (vec4_t){ndc, 1.0f});
  world = mat4_mulv(inv_view, world);

  if (fabs(world.w) >= FLT_EPSILON) {
    world.xyz = vec3_divs(world.xyz, world.w);
  }

  return world.xyz;
}

vec3_t eg_camera_world_to_ndc(eg_camera_t *camera, vec3_t world) {
  vec4_t ndc = mat4_mulv(
      camera->uniform.proj,
      mat4_mulv(camera->uniform.view, (vec4_t){world, 1.0f}));

  if (fabs(ndc.w) >= FLT_EPSILON) {
    ndc.xyz = vec3_divs(ndc.xyz, ndc.w);
  }

  return ndc.xyz;
}
