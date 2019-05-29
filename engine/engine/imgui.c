#include "imgui.h"

#include <fstd_util.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>
#include <renderer/context.h>
#include <renderer/util.h>
#include <renderer/window.h>

static void check_vk_result_fn(VkResult result) {
  if (result != VK_SUCCESS) {
    RE_LOG_FATAL("Failed to initialize IMGUI!");
    abort();
  }
}

void eg_imgui_init(re_window_t *window, re_render_target_t *render_target) {
  igCreateContext(NULL);
  igGetIO();

  ImGui_ImplGlfw_InitForVulkan(window->glfw_window, false);

  // Setup Vulkan binding
  ImGui_ImplVulkan_InitInfo init_info = {0};
  init_info.Instance = g_ctx.instance;
  init_info.PhysicalDevice = g_ctx.physical_device;
  init_info.Device = g_ctx.device;
  init_info.QueueFamily = g_ctx.graphics_queue_family_index;
  init_info.Queue = g_ctx.graphics_queue;
  init_info.PipelineCache = VK_NULL_HANDLE;
  init_info.DescriptorPool = g_ctx.descriptor_pool;
  init_info.SampleCount = render_target->sample_count;
  init_info.Allocator = NULL;
  init_info.CheckVkResultFn = check_vk_result_fn;
  ImGui_ImplVulkan_Init(&init_info, render_target->render_pass);

  // Setup style
  igStyleColorsDark(NULL);

  // Upload Fonts
  {
    // Use any command queue
    VkCommandPool command_pool = g_ctx.graphics_command_pool;
    re_cmd_buffer_t *cmd_buffer = re_window_get_current_command_buffer(window);

    VK_CHECK(vkResetCommandPool(g_ctx.device, command_pool, 0));

    re_begin_cmd_buffer(
        cmd_buffer,
        &(re_cmd_buffer_begin_info_t){.usage =
                                          RE_CMD_BUFFER_USAGE_ONE_TIME_SUBMIT});

    ImGui_ImplVulkan_CreateFontsTexture(cmd_buffer->cmd_buffer);

    VkSubmitInfo end_info = {0};
    end_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    end_info.commandBufferCount = 1;
    end_info.pCommandBuffers = &cmd_buffer->cmd_buffer;

    re_end_cmd_buffer(cmd_buffer);

    mtx_lock(&g_ctx.queue_mutex);
    VK_CHECK(vkQueueSubmit(g_ctx.graphics_queue, 1, &end_info, VK_NULL_HANDLE));
    mtx_unlock(&g_ctx.queue_mutex);

    VK_CHECK(vkDeviceWaitIdle(g_ctx.device));

    ImGui_ImplVulkan_InvalidateFontUploadObjects();
  }
}

void eg_imgui_begin() {
  ImGui_ImplVulkan_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  igNewFrame();
}

void eg_imgui_end() { igRender(); }

void eg_imgui_draw(re_cmd_buffer_t *cmd_buffer) {
  ImGui_ImplVulkan_RenderDrawData(igGetDrawData(), cmd_buffer->cmd_buffer);
}

void eg_imgui_process_event(const re_event_t *event) {
  switch (event->type) {
  case RE_EVENT_BUTTON_PRESSED: {
    ImGui_ImplGlfw_MouseButtonCallback(
        event->window->glfw_window,
        event->mouse.button,
        GLFW_PRESS,
        event->mouse.mods);
    break;
  }
  case RE_EVENT_BUTTON_RELEASED: {
    ImGui_ImplGlfw_MouseButtonCallback(
        event->window->glfw_window,
        event->mouse.button,
        GLFW_RELEASE,
        event->mouse.mods);
    break;
  }
  case RE_EVENT_SCROLLED: {
    ImGui_ImplGlfw_ScrollCallback(
        event->window->glfw_window, event->scroll.x, event->scroll.y);
    break;
  }
  case RE_EVENT_KEY_PRESSED: {
    ImGui_ImplGlfw_KeyCallback(
        event->window->glfw_window,
        event->keyboard.key,
        event->keyboard.scancode,
        GLFW_PRESS,
        event->keyboard.mods);
    break;
  }
  case RE_EVENT_KEY_REPEATED: {
    ImGui_ImplGlfw_KeyCallback(
        event->window->glfw_window,
        event->keyboard.key,
        event->keyboard.scancode,
        GLFW_REPEAT,
        event->keyboard.mods);
    break;
  }
  case RE_EVENT_KEY_RELEASED: {
    ImGui_ImplGlfw_KeyCallback(
        event->window->glfw_window,
        event->keyboard.key,
        event->keyboard.scancode,
        GLFW_RELEASE,
        event->keyboard.mods);
    break;
  }
  case RE_EVENT_CODEPOINT_INPUT: {
    ImGui_ImplGlfw_CharCallback(event->window->glfw_window, event->codepoint);
    break;
  }
  default: break;
  }
}

void eg_imgui_destroy() {
  VK_CHECK(vkDeviceWaitIdle(g_ctx.device));

  ImGui_ImplVulkan_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  igDestroyContext(NULL);
}
