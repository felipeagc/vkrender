#include "imgui.hpp"

#include "context.hpp"
#include "util.hpp"
#include "window.hpp"
#include <imgui/imgui.h>
#include <imgui/imgui_impl_sdl.h>
#include <imgui/imgui_impl_vulkan.h>

static inline void create_descriptor_pool(re_imgui_t *imgui) {
  VkDescriptorPoolSize imguiPoolSizes[] = {
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

  VkDescriptorPoolCreateInfo createInfo = {
      VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,           // sType
      NULL,                                                    // pNext
      VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,       // flags
      1000 * static_cast<uint32_t>(ARRAYSIZE(imguiPoolSizes)), // maxSets
      static_cast<uint32_t>(ARRAYSIZE(imguiPoolSizes)),        // poolSizeCount
      imguiPoolSizes,                                          // pPoolSizes
  };

  VK_CHECK(vkCreateDescriptorPool(
      g_ctx.device, &createInfo, NULL, &imgui->descriptor_pool));
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
      throw std::runtime_error("Failed to initialize IMGUI!");
    }
  };
  ImGui_ImplVulkan_Init(&init_info, imgui->window->render_target.render_pass);

  // Setup style
  ImGui::StyleColorsDark();

  // Upload Fonts
  {
    // Use any command queue
    VkCommandPool commandPool = g_ctx.graphics_command_pool;
    auto commandBuffer = re_window_get_current_command_buffer(imgui->window);

    VK_CHECK(vkResetCommandPool(g_ctx.device, commandPool, 0));
    VkCommandBufferBeginInfo beginInfo = {
        VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, // sType
        NULL,                                        // pNext
        VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, // flags
        NULL,                                        // pInheritanceInfo
    };

    VK_CHECK(vkBeginCommandBuffer(commandBuffer, &beginInfo));

    ImGui_ImplVulkan_CreateFontsTexture(commandBuffer);

    VkSubmitInfo endInfo = {};
    endInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    endInfo.commandBufferCount = 1;
    endInfo.pCommandBuffers = &commandBuffer;

    VK_CHECK(vkEndCommandBuffer(commandBuffer));

    pthread_mutex_lock(&g_ctx.queue_mutex);
    VK_CHECK(vkQueueSubmit(g_ctx.graphics_queue, 1, &endInfo, VK_NULL_HANDLE));
    pthread_mutex_unlock(&g_ctx.queue_mutex);

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
  auto command_buffer = re_window_get_current_command_buffer(imgui->window);

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
