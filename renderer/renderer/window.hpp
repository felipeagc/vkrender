#pragma once

#include "common.hpp"
#include "glm.hpp"
#include "render_target.hpp"
#include <SDL2/SDL.h>
#include <vulkan/vk_mem_alloc.h>
#include <vulkan/vulkan.h>

struct re_window_t {
  glm::vec4 clear_color;

  re_render_target_t render_target;
  SDL_Window *sdl_window;

  bool should_close;

  double delta_time;
  uint32_t time_before;

  VkSurfaceKHR surface;

  VkSampleCountFlagBits max_samples;

  VkFormat depth_format;

  // @note: might wanna make one of these per frame
  struct {
    VkImage image;
    VmaAllocation allocation;
    VkImageView view;
  } depth_stencil;

  struct FrameResources {
    VkSemaphore image_available_semaphore;
    VkSemaphore rendering_finished_semaphore;
    VkFence fence;

    VkFramebuffer framebuffer;

    VkCommandBuffer command_buffer;
  } frame_resources[renderer::MAX_FRAMES_IN_FLIGHT];

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
};

bool re_window_init(
    re_window_t *window, const char *title, uint32_t width, uint32_t height);

void re_window_get_size(
    const re_window_t *window, uint32_t *width, uint32_t *height);

bool re_window_poll_event(re_window_t *window, SDL_Event *event);

void re_window_begin_frame(re_window_t *window);
void re_window_end_frame(re_window_t *window);

void re_window_begin_render_pass(re_window_t *window);
void re_window_end_render_pass(re_window_t *window);

bool re_window_get_relative_mouse(const re_window_t *window);
void re_window_set_relative_mouse(re_window_t *window, bool relative);

void re_window_get_mouse_state(const re_window_t *window, int *x, int *y);
void re_window_get_relative_mouse_state(const re_window_t *window, int *x, int *y);

void re_window_warp_mouse(re_window_t *window, int x, int y);

bool re_window_is_mouse_left_pressed(const re_window_t *window);
bool re_window_is_mouse_right_pressed(const re_window_t *window);

bool re_window_is_scancode_pressed(
    const re_window_t *window, SDL_Scancode scancode);

VkCommandBuffer re_window_get_current_command_buffer(const re_window_t *window);

void re_window_destroy(re_window_t *window);
