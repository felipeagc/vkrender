#include "imgui.h"

#include "context.h"
#include "util.h"
#include "window.h"
#include <fstd_util.h>
#include <imgui_impl_sdl.h>
#include <imgui_impl_vulkan.h>
#include <util/log.h>

static void check_vk_result_fn(VkResult result) {
    if (result != VK_SUCCESS) {
      ut_log_fatal("Failed to initialize IMGUI!");
      abort();
    }
}

void re_imgui_init(re_window_t *window) {
  igCreateContext(NULL);
  igGetIO();

  ImGui_ImplSDL2_InitForVulkan(window->sdl_window);

  // Setup Vulkan binding
  ImGui_ImplVulkan_InitInfo init_info = {0};
  init_info.Instance = g_ctx.instance;
  init_info.PhysicalDevice = g_ctx.physical_device;
  init_info.Device = g_ctx.device;
  init_info.QueueFamily = g_ctx.graphics_queue_family_index;
  init_info.Queue = g_ctx.graphics_queue;
  init_info.PipelineCache = VK_NULL_HANDLE;
  init_info.DescriptorPool = g_ctx.descriptor_pool;
  init_info.Allocator = NULL;
  init_info.CheckVkResultFn = check_vk_result_fn;
  ImGui_ImplVulkan_Init(&init_info, window->render_target.render_pass);

  // Setup style
  igStyleColorsDark(NULL);

  // Upload Fonts
  {
    // Use any command queue
    VkCommandPool command_pool = g_ctx.graphics_command_pool;
    VkCommandBuffer command_buffer =
        re_window_get_current_command_buffer(window);

    VK_CHECK(vkResetCommandPool(g_ctx.device, command_pool, 0));
    VkCommandBufferBeginInfo begin_info = {
        VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, // sType
        NULL,                                        // pNext
        VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, // flags
        NULL,                                        // pInheritanceInfo
    };

    VK_CHECK(vkBeginCommandBuffer(command_buffer, &begin_info));

    ImGui_ImplVulkan_CreateFontsTexture(command_buffer);

    VkSubmitInfo end_info = {0};
    end_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    end_info.commandBufferCount = 1;
    end_info.pCommandBuffers = &command_buffer;

    VK_CHECK(vkEndCommandBuffer(command_buffer));

    mtx_lock(&g_ctx.queue_mutex);
    VK_CHECK(vkQueueSubmit(g_ctx.graphics_queue, 1, &end_info, VK_NULL_HANDLE));
    mtx_unlock(&g_ctx.queue_mutex);

    VK_CHECK(vkDeviceWaitIdle(g_ctx.device));

    ImGui_ImplVulkan_InvalidateFontUploadObjects();
  }
}

void re_imgui_begin(re_window_t *window) {
  ImGui_ImplVulkan_NewFrame();
  ImGui_ImplSDL2_NewFrame(window->sdl_window);
  igNewFrame();
}

void re_imgui_end() { igRender(); }

void re_imgui_draw(re_window_t *window) {
  VkCommandBuffer command_buffer = re_window_get_current_command_buffer(window);

  ImGui_ImplVulkan_RenderDrawData(igGetDrawData(), command_buffer);
}

void re_imgui_process_event(SDL_Event *event) {
  ImGui_ImplSDL2_ProcessEvent(event);
}

void re_imgui_destroy() {
  VK_CHECK(vkDeviceWaitIdle(g_ctx.device));

  ImGui_ImplVulkan_Shutdown();
  ImGui_ImplSDL2_Shutdown();
  igDestroyContext(NULL);
}
