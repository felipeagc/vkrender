#pragma once

#include "common.h"
#include "render_target.h"
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <gmath.h>
#include <stdbool.h>
#include <vulkan/vk_mem_alloc.h>
#include <vulkan/vulkan.h>

typedef struct re_window_t re_window_t;

typedef void (*re_framebuffer_resize_callback_t)(re_window_t *, int, int);
typedef void (*re_mouse_button_callback_t)(re_window_t *, int, int, int);
typedef void (*re_scroll_callback_t)(re_window_t *, double, double);
typedef void (*re_key_callback_t)(re_window_t *, int, int, int, int);
typedef void (*re_char_callback_t)(re_window_t *, unsigned int);

typedef struct re_frame_resources_t {
  VkSemaphore image_available_semaphore;
  VkSemaphore rendering_finished_semaphore;
  VkFence fence;

  VkFramebuffer framebuffer;

  VkCommandBuffer command_buffer;
} re_frame_resources_t;

typedef struct re_window_t {
  vec4_t clear_color;

  re_render_target_t render_target;
  GLFWwindow *glfw_window;

  double delta_time;
  double time_before;

  VkSurfaceKHR surface;

  VkSampleCountFlagBits max_samples;

  VkFormat depth_format;

  // @NOTE: might wanna make one of these per frame
  struct {
    VkImage image;
    VmaAllocation allocation;
    VkImageView view;
  } depth_stencil;

  re_frame_resources_t frame_resources[RE_MAX_FRAMES_IN_FLIGHT];

  // Current frame (capped by MAX_FRAMES_IN_FLIGHT)
  uint32_t current_frame;
  // Index of the current swapchain image
  uint32_t current_image_index;

  VkSwapchainKHR swapchain;
  VkFormat swapchain_image_format;
  VkExtent2D swapchain_extent;

  uint32_t swapchain_image_count;
  VkImage *swapchain_images;
  VkImageView *swapchain_image_views;

  void *user_ptr;
  re_framebuffer_resize_callback_t framebuffer_resize_callback;
  re_mouse_button_callback_t mouse_button_callback;
  re_scroll_callback_t scroll_callback;
  re_key_callback_t key_callback;
  re_char_callback_t char_callback;
} re_window_t;

bool re_window_init(
    re_window_t *window, const char *title, uint32_t width, uint32_t height);

void re_window_get_size(
    const re_window_t *window, uint32_t *width, uint32_t *height);

void re_window_poll_events(re_window_t *window);

bool re_window_should_close(re_window_t *window);

void re_window_begin_frame(re_window_t *window);
void re_window_end_frame(re_window_t *window);

void re_window_begin_render_pass(re_window_t *window);
void re_window_end_render_pass(re_window_t *window);

void re_window_set_input_mode(const re_window_t *window, int mode, int value);
int re_window_get_input_mode(const re_window_t *window, int mode);

void re_window_get_cursor_pos(const re_window_t *window, double *x, double *y);
void re_window_set_cursor_pos(re_window_t *window, double x, double y);

bool re_window_is_mouse_left_pressed(const re_window_t *window);
bool re_window_is_mouse_right_pressed(const re_window_t *window);

bool re_window_is_key_pressed(const re_window_t *window, int key);

VkCommandBuffer re_window_get_current_command_buffer(const re_window_t *window);

void re_window_destroy(re_window_t *window);
