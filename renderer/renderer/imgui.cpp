#include "imgui.hpp"

#include "context.hpp"
#include "util.hpp"
#include "window.hpp"
#include <imgui/imgui.h>
#include <imgui/imgui_impl_sdl.h>
#include <imgui/imgui_impl_vulkan.h>
#include <util/log.h>
#include <util/array.h>

static inline void create_descriptor_pool(re_imgui_t *imgui) {
  VkDescriptorPoolSize imgui_pool_sizes[] = {
      {VK_DESCRIPTOR_TYPE_SAMPLER, 1000},
      {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000},
      {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000},
      {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000},
      {VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000},
      {VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000},
      {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000},
      {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000},
      {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000},
      {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000},
      {VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000},
  };

  VkDescriptorPoolCreateInfo create_info = {
      VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,     // sType
      NULL,                                              // pNext
      VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT, // flags
      1000 * (uint32_t)ARRAYSIZE(imgui_pool_sizes),      // maxSets
      (uint32_t)ARRAYSIZE(imgui_pool_sizes),             // poolSizeCount
      imgui_pool_sizes,                                  // pPoolSizes
  };

  VK_CHECK(vkCreateDescriptorPool(
      g_ctx.device, &create_info, NULL, &imgui->descriptor_pool));
}

static inline void destroy_descriptor_pool(re_imgui_t *imgui) {
  VK_CHECK(vkDeviceWaitIdle(g_ctx.device));
  if (imgui->descriptor_pool != VK_NULL_HANDLE) {
    vkDestroyDescriptorPool(g_ctx.device, imgui->descriptor_pool, NULL);
  }
}

void re_imgui_init(re_imgui_t *imgui, re_window_t *window) {
  imgui->window = window;
  create_descriptor_pool(imgui);

  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
  (void)io;

  ImGui_ImplSDL2_InitForVulkan(imgui->window->sdl_window);

  // Setup Vulkan binding
  ImGui_ImplVulkan_InitInfo init_info = {};
  init_info.Instance = g_ctx.instance;
  init_info.PhysicalDevice = g_ctx.physical_device;
  init_info.Device = g_ctx.device;
  init_info.QueueFamily = g_ctx.graphics_queue_family_index;
  init_info.Queue = g_ctx.graphics_queue;
  init_info.PipelineCache = VK_NULL_HANDLE;
  init_info.DescriptorPool = imgui->descriptor_pool;
  init_info.Allocator = NULL;
  init_info.CheckVkResultFn = [](VkResult result) {
    if (result != VK_SUCCESS) {
      ut_log_fatal("Failed to initialize IMGUI!");
      abort();
    }
  };
  ImGui_ImplVulkan_Init(&init_info, imgui->window->render_target.render_pass);

  // Setup style
  ImGui::StyleColorsDark();

  // Upload Fonts
  {
    // Use any command queue
    VkCommandPool command_pool = g_ctx.graphics_command_pool;
    VkCommandBuffer command_buffer =
        re_window_get_current_command_buffer(imgui->window);

    VK_CHECK(vkResetCommandPool(g_ctx.device, command_pool, 0));
    VkCommandBufferBeginInfo begin_info = {
        VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, // sType
        NULL,                                        // pNext
        VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, // flags
        NULL,                                        // pInheritanceInfo
    };

    VK_CHECK(vkBeginCommandBuffer(command_buffer, &begin_info));

    ImGui_ImplVulkan_CreateFontsTexture(command_buffer);

    VkSubmitInfo end_info = {};
    end_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    end_info.commandBufferCount = 1;
    end_info.pCommandBuffers = &command_buffer;

    VK_CHECK(vkEndCommandBuffer(command_buffer));

    ut_mutex_lock(&g_ctx.queue_mutex);
    VK_CHECK(vkQueueSubmit(g_ctx.graphics_queue, 1, &end_info, VK_NULL_HANDLE));
    ut_mutex_unlock(&g_ctx.queue_mutex);

    VK_CHECK(vkDeviceWaitIdle(g_ctx.device));

    ImGui_ImplVulkan_InvalidateFontUploadObjects();
  }
}

void re_imgui_begin(re_imgui_t *imgui) {
  ImGui_ImplVulkan_NewFrame();
  ImGui_ImplSDL2_NewFrame(imgui->window->sdl_window);
  ImGui::NewFrame();
}

void re_imgui_end(re_imgui_t *) { ImGui::Render(); }

void re_imgui_draw(re_imgui_t *imgui) {
  VkCommandBuffer command_buffer =
      re_window_get_current_command_buffer(imgui->window);

  ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), command_buffer);
}

void re_imgui_process_event(re_imgui_t *, SDL_Event *event) {
  ImGui_ImplSDL2_ProcessEvent(event);
}

void re_imgui_destroy(re_imgui_t *imgui) {
  VK_CHECK(vkDeviceWaitIdle(g_ctx.device));

  ImGui_ImplVulkan_Shutdown();
  ImGui_ImplSDL2_Shutdown();
  ImGui::DestroyContext();

  destroy_descriptor_pool(imgui);
}
