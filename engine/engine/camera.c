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
}

void eg_camera_update(
    eg_camera_t *camera,
    re_cmd_buffer_t *cmd_buffer,
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
}

void eg_camera_bind(
    eg_camera_t *camera,
    re_cmd_buffer_t *cmd_buffer,
    struct re_pipeline_t *pipeline,
    uint32_t set) {
  void *mapping =
      re_cmd_bind_uniform(cmd_buffer, set, 0, sizeof(camera->uniform));
  memcpy(mapping, &camera->uniform, sizeof(camera->uniform));

  re_cmd_bind_descriptor_set(cmd_buffer, pipeline, set);
}

void eg_camera_destroy(eg_camera_t *camera) {}

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
