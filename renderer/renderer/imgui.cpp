#include "imgui.hpp"

#include "context.hpp"
#include "util.hpp"
#include "window.hpp"
#include <imgui/imgui.h>
#include <imgui/imgui_impl_sdl.h>
#include <imgui/imgui_impl_vulkan.h>

using namespace renderer;

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
      nullptr,                                                 // pNext
      VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,       // flags
      1000 * static_cast<uint32_t>(ARRAYSIZE(imguiPoolSizes)), // maxSets
      static_cast<uint32_t>(ARRAYSIZE(imguiPoolSizes)),        // poolSizeCount
      imguiPoolSizes,                                          // pPoolSizes
  };

  VK_CHECK(vkCreateDescriptorPool(
      ctx().m_device, &createInfo, nullptr, &imgui->descriptor_pool));
}

static inline void destroy_descriptor_pool(re_imgui_t *imgui) {
  VK_CHECK(vkDeviceWaitIdle(ctx().m_device));
  if (imgui->descriptor_pool != VK_NULL_HANDLE) {
    vkDestroyDescriptorPool(ctx().m_device, imgui->descriptor_pool, nullptr);
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
  init_info.Instance = ctx().m_instance;
  init_info.PhysicalDevice = ctx().m_physicalDevice;
  init_info.Device = ctx().m_device;
  init_info.QueueFamily = ctx().m_graphicsQueueFamilyIndex;
  init_info.Queue = ctx().m_graphicsQueue;
  init_info.PipelineCache = VK_NULL_HANDLE;
  init_info.DescriptorPool = imgui->descriptor_pool;
  init_info.Allocator = nullptr;
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
    VkCommandPool commandPool = ctx().m_graphicsCommandPool;
    auto commandBuffer = re_window_get_current_command_buffer(imgui->window);

    VK_CHECK(vkResetCommandPool(ctx().m_device, commandPool, 0));
    VkCommandBufferBeginInfo beginInfo = {
        VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, // sType
        nullptr,                                     // pNext
        VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, // flags
        nullptr,                                     // pInheritanceInfo
    };

    VK_CHECK(vkBeginCommandBuffer(commandBuffer, &beginInfo));

    ImGui_ImplVulkan_CreateFontsTexture(commandBuffer);

    VkSubmitInfo endInfo = {};
    endInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    endInfo.commandBufferCount = 1;
    endInfo.pCommandBuffers = &commandBuffer;

    VK_CHECK(vkEndCommandBuffer(commandBuffer));

    renderer::ctx().m_queueMutex.lock();
    VK_CHECK(vkQueueSubmit(ctx().m_graphicsQueue, 1, &endInfo, VK_NULL_HANDLE));
    renderer::ctx().m_queueMutex.unlock();

    VK_CHECK(vkDeviceWaitIdle(ctx().m_device));

    ImGui_ImplVulkan_InvalidateFontUploadObjects();
  }
}

void re_imgui_begin(re_imgui_t *imgui) {
  ImGui_ImplVulkan_NewFrame();
  ImGui_ImplSDL2_NewFrame(imgui->window->sdl_window);
  ImGui::NewFrame();
}

void re_imgui_end(re_imgui_t *) {
  ImGui::Render();
}

void re_imgui_draw(re_imgui_t *imgui) {
  auto command_buffer = re_window_get_current_command_buffer(imgui->window);

  ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), command_buffer);
}

void re_imgui_process_event(re_imgui_t *, SDL_Event *event) {
  ImGui_ImplSDL2_ProcessEvent(event);
}

void re_imgui_destroy(re_imgui_t *imgui) {
  ImGui_ImplVulkan_Shutdown();
  ImGui_ImplSDL2_Shutdown();
  ImGui::DestroyContext();

  destroy_descriptor_pool(imgui);
}
