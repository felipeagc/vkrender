#include "imgui.hpp"

#include "context.hpp"
#include "util.hpp"
#include "window.hpp"
#include <imgui/imgui.h>
#include <imgui/imgui_impl_sdl.h>
#include <imgui/imgui_impl_vulkan.h>

using namespace renderer;

ImGuiRenderer::ImGuiRenderer(Window &window) : m_window(window) {
  this->createDescriptorPool();

  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
  (void)io;

  ImGui_ImplSDL2_InitForVulkan(m_window.m_window);

  // Setup Vulkan binding
  ImGui_ImplVulkan_InitInfo init_info = {};
  init_info.Instance = ctx().m_instance;
  init_info.PhysicalDevice = ctx().m_physicalDevice;
  init_info.Device = ctx().m_device;
  init_info.QueueFamily = ctx().m_graphicsQueueFamilyIndex;
  init_info.Queue = ctx().m_graphicsQueue;
  init_info.PipelineCache = VK_NULL_HANDLE;
  init_info.DescriptorPool = m_descriptorPool;
  init_info.Allocator = nullptr;
  init_info.CheckVkResultFn = [](VkResult result) {
    if (result != VK_SUCCESS) {
      throw std::runtime_error("Failed to initialize IMGUI!");
    }
  };
  ImGui_ImplVulkan_Init(&init_info, m_window.m_renderPass);

  // Setup style
  ImGui::StyleColorsDark();

  // Upload Fonts
  {
    // Use any command queue
    VkCommandPool commandPool = ctx().m_graphicsCommandPool;
    auto commandBuffer = m_window.getCurrentCommandBuffer();

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

    VK_CHECK(vkQueueSubmit(ctx().m_graphicsQueue, 1, &endInfo, VK_NULL_HANDLE));

    VK_CHECK(vkDeviceWaitIdle(ctx().m_device));

    ImGui_ImplVulkan_InvalidateFontUploadObjects();
  }
}

ImGuiRenderer::~ImGuiRenderer() {
  ImGui_ImplVulkan_Shutdown();
  ImGui_ImplSDL2_Shutdown();
  ImGui::DestroyContext();

  this->destroyDescriptorPool();
}

void ImGuiRenderer::begin() {
  ImGui_ImplVulkan_NewFrame();
  ImGui_ImplSDL2_NewFrame(m_window.m_window);
  ImGui::NewFrame();
}

void ImGuiRenderer::end() { ImGui::Render(); }

void ImGuiRenderer::draw() {
  auto commandBuffer = m_window.getCurrentCommandBuffer();

  ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);
}

void ImGuiRenderer::processEvent(SDL_Event *event) {
  ImGui_ImplSDL2_ProcessEvent(event);
}

void ImGuiRenderer::createDescriptorPool() {
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
      ctx().m_device, &createInfo, nullptr, &m_descriptorPool));
}

void ImGuiRenderer::destroyDescriptorPool() {
  VK_CHECK(vkDeviceWaitIdle(ctx().m_device));
  if (m_descriptorPool != VK_NULL_HANDLE) {
    vkDestroyDescriptorPool(ctx().m_device, m_descriptorPool, nullptr);
  }
}
