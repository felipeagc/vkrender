#include "picker.h"
#include <renderer/context.h>
#include <renderer/window.h>
#include <string.h>

void eg_picker_init(
    eg_picker_t *picker, re_window_t *window, uint32_t width, uint32_t height) {
  picker->window = window;

  re_canvas_init(
      &picker->canvas,
      &(re_canvas_options_t){
          .width = width,
          .height = height,
          .color_format = VK_FORMAT_R32_UINT,
          .clear_color = {
              .uint32 = {UINT32_MAX, UINT32_MAX, UINT32_MAX, UINT32_MAX},
          }});

  VK_CHECK(vkCreateFence(
      g_ctx.device,
      &(VkFenceCreateInfo){
          .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
      },
      NULL,
      &picker->fence));

  re_allocate_cmd_buffers(
      &(re_cmd_buffer_alloc_info_t){
          .pool = g_ctx.graphics_command_pool,
          .count = 1,
          .level = RE_CMD_BUFFER_LEVEL_PRIMARY,
      },
      &picker->cmd_buffer);

  re_buffer_init(
      &picker->pixel_buffer,
      &(re_buffer_options_t){
          .type = RE_BUFFER_TYPE_TRANSFER,
          .size = sizeof(uint32_t),
      });
}

void eg_picker_destroy(eg_picker_t *picker) {
  VK_CHECK(vkDeviceWaitIdle(g_ctx.device));

  re_canvas_destroy(&picker->canvas);
  re_buffer_destroy(&picker->pixel_buffer);
  re_free_cmd_buffers(g_ctx.graphics_command_pool, 1, &picker->cmd_buffer);
  vkDestroyFence(g_ctx.device, picker->fence, NULL);
}

void eg_picker_resize(eg_picker_t *picker, uint32_t width, uint32_t height) {
  re_canvas_resize(&picker->canvas, width, height);
}

eg_cmd_info_t eg_picker_begin(eg_picker_t *picker) {
  vkResetFences(g_ctx.device, 1, &picker->fence);

  re_begin_cmd_buffer(
      picker->cmd_buffer,
      &(re_cmd_buffer_begin_info_t){
          .usage = RE_CMD_BUFFER_USAGE_ONE_TIME_SUBMIT,
      });

  eg_cmd_info_t cmd_info = {
      .frame_index = picker->window->current_frame,
      .cmd_buffer = picker->cmd_buffer,
  };

  re_canvas_begin(&picker->canvas, picker->cmd_buffer);

  return cmd_info;
}

void eg_picker_end(eg_picker_t *picker) {
  re_canvas_end(&picker->canvas, picker->cmd_buffer);

  re_end_cmd_buffer(picker->cmd_buffer);

  VK_CHECK(vkQueueSubmit(
      g_ctx.graphics_queue,
      1,
      &(VkSubmitInfo){
          .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
          .commandBufferCount = 1,
          .pCommandBuffers = &picker->cmd_buffer,
      },
      picker->fence));

  VK_CHECK(
      vkWaitForFences(g_ctx.device, 1, &picker->fence, VK_TRUE, UINT64_MAX));
}

uint32_t eg_picker_get(eg_picker_t *picker, uint32_t x, uint32_t y) {
  re_image_transfer_to_buffer(
      picker->canvas.resources[0].color.image,
      &picker->pixel_buffer,
      g_ctx.transient_command_pool,
      x,
      y,
      1,
      1,
      0,
      0);

  void *mapping;
  re_buffer_map_memory(&picker->pixel_buffer, &mapping);

  uint32_t selected;
  memcpy(&selected, mapping, sizeof(uint32_t));

  re_buffer_unmap_memory(&picker->pixel_buffer);

  return selected;
}
