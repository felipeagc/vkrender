#include "window.hpp"
#include "context.hpp"
#include "util.hpp"
#include <SDL2/SDL_vulkan.h>
#include <chrono>
#include <fstl/logging.hpp>
#include <imgui/imgui.h>
#include <imgui/imgui_impl_sdl.h>
#include <imgui/imgui_impl_vulkan.h>

using namespace renderer;

Window::Window(const char *title, uint32_t width, uint32_t height) {
  auto subsystems = SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_TIMER;
  if (subsystems == SDL_WasInit(0)) {
    SDL_Init(subsystems);
  }

  m_window = SDL_CreateWindow(
      title,
      SDL_WINDOWPOS_CENTERED,
      SDL_WINDOWPOS_CENTERED,
      width,
      height,
      SDL_WINDOW_SHOWN | SDL_WINDOW_VULKAN);

  if (m_window == nullptr) {
    throw std::runtime_error("Failed to create SDL window");
  }

  SDL_SetWindowResizable(m_window, SDL_TRUE);

  uint32_t sdlExtensionCount = 0;
  SDL_Vulkan_GetInstanceExtensions(m_window, &sdlExtensionCount, nullptr);
  std::vector<const char *> sdlExtensions(sdlExtensionCount);
  SDL_Vulkan_GetInstanceExtensions(
      m_window, &sdlExtensionCount, sdlExtensions.data());

  // These context initialization functions only run if the context is
  // uninitialized
  ctx().preInitialize(sdlExtensions);

  this->createVulkanSurface();

  // Lazily create vulkan context stuff
  ctx().lateInitialize(m_surface);

  VkBool32 supported;
  vkGetPhysicalDeviceSurfaceSupportKHR(
      ctx().m_physicalDevice,
      ctx().m_presentQueueFamilyIndex,
      m_surface,
      &supported);
  if (!supported) {
    throw std::runtime_error(
        "Selected present queue does not support this window's surface");
  }

  m_maxSampleCount = ctx().getMaxUsableSampleCount();

  this->createSyncObjects();

  this->createSwapchain(this->getWidth(), this->getHeight());
  this->createSwapchainImageViews();

  this->allocateGraphicsCommandBuffers();

  this->createDepthStencilResources();

  this->createRenderPass();

  this->createImguiRenderPass();

  this->initImgui();
}

Window::~Window() {
  VK_CHECK(vkDeviceWaitIdle(ctx().m_device));

  ImGui_ImplVulkan_Shutdown();
  ImGui_ImplSDL2_Shutdown();
  ImGui::DestroyContext();

  this->destroyResizables();

  for (auto &swapchainImageView : m_swapchainImageViews) {
    vkDestroyImageView(ctx().m_device, swapchainImageView, nullptr);
  }

  vkDestroySwapchainKHR(ctx().m_device, m_swapchain, nullptr);

  for (auto &frameResource : m_frameResources) {
    vkDestroyFramebuffer(ctx().m_device, frameResource.framebuffer, nullptr);
    vkDestroyFramebuffer(
        ctx().m_device, frameResource.imguiFramebuffer, nullptr);
    vkDestroySemaphore(
        ctx().m_device, frameResource.imageAvailableSemaphore, nullptr);
    vkDestroySemaphore(
        ctx().m_device, frameResource.renderingFinishedSemaphore, nullptr);
    vkDestroyFence(ctx().m_device, frameResource.fence, nullptr);
  }

  vkDestroySurfaceKHR(ctx().m_instance, m_surface, nullptr);

  SDL_DestroyWindow(m_window);
}

bool Window::pollEvent(SDL_Event *event) {
  bool result = SDL_PollEvent(event);

  switch (event->type) {
  case SDL_WINDOWEVENT:
    switch (event->window.event) {
    case SDL_WINDOWEVENT_RESIZED:
      this->updateSize();
      break;
    }
    break;
  }

  if (result)
    ImGui_ImplSDL2_ProcessEvent(event);

  return result;
}

void Window::beginFrame() {
  m_timeBefore = std::chrono::high_resolution_clock::now();

  // Begin
  vkWaitForFences(
      ctx().m_device,
      1,
      &m_frameResources[m_currentFrame].fence,
      VK_TRUE,
      UINT64_MAX);

  vkResetFences(ctx().m_device, 1, &m_frameResources[m_currentFrame].fence);

  if (vkAcquireNextImageKHR(
          ctx().m_device,
          m_swapchain,
          UINT64_MAX,
          m_frameResources[m_currentFrame].imageAvailableSemaphore,
          VK_NULL_HANDLE,
          &m_currentImageIndex) == VK_ERROR_OUT_OF_DATE_KHR) {
    this->updateSize();
  }

  this->imguiBeginFrame();

  VkImageSubresourceRange imageSubresourceRange{
      VK_IMAGE_ASPECT_COLOR_BIT, // aspectMask
      0,                         // baseMipLevel
      1,                         // levelCount
      0,                         // baseArrayLayer
      1,                         // layerCount
  };

  this->regenFramebuffer(
      m_frameResources[m_currentFrame].framebuffer,
      m_swapchainImageViews[m_currentImageIndex]);

  this->regenImguiFramebuffer(
      m_frameResources[m_currentFrame].imguiFramebuffer,
      m_swapchainImageViews[m_currentImageIndex]);

  VkCommandBufferBeginInfo beginInfo = {};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
  beginInfo.pInheritanceInfo = nullptr;

  auto &commandBuffer = m_frameResources[m_currentFrame].commandBuffer;

  VK_CHECK(vkBeginCommandBuffer(commandBuffer, &beginInfo));

  if (ctx().m_presentQueue != ctx().m_graphicsQueue) {
    VkImageMemoryBarrier barrierFromPresentToDraw = {
        VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,   // sType
        nullptr,                                  // pNext
        VK_ACCESS_MEMORY_READ_BIT,                // srcAccessMask
        VK_ACCESS_MEMORY_READ_BIT,                // dstAccessMask
        VK_IMAGE_LAYOUT_UNDEFINED,                // oldLayout
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, // newLayout
        ctx().m_presentQueueFamilyIndex,          // srcQueueFamilyIndex
        ctx().m_graphicsQueueFamilyIndex,         // dstQueueFamilyIndex
        m_swapchainImages[m_currentImageIndex],   // image
        imageSubresourceRange,                    // subresourceRange
    };

    vkCmdPipelineBarrier(
        commandBuffer,
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        0,
        0,
        nullptr,
        0,
        nullptr,
        1,
        &barrierFromPresentToDraw);
  }
}

void Window::endFrame() {
  auto &commandBuffer = m_frameResources[m_currentFrame].commandBuffer;

  {
    VkRenderPassBeginInfo imguiRenderPassBeginInfo = {
        VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,          // sType
        nullptr,                                           // pNext
        m_imguiRenderPass,                                 // renderPass
        m_frameResources[m_currentFrame].imguiFramebuffer, // framebuffer
        {{0, 0}, m_swapchainExtent},                       // renderArea
        0,                                                 // clearValueCount
        nullptr,                                           // pClearValues
    };

    vkCmdBeginRenderPass(
        commandBuffer, &imguiRenderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

    VkViewport viewport{
        0.0f,                                         // x
        0.0f,                                         // y
        static_cast<float>(m_swapchainExtent.width),  // width
        static_cast<float>(m_swapchainExtent.height), // height
        0.0f,                                         // minDepth
        1.0f,                                         // maxDepth
    };

    VkRect2D scissor{{0, 0}, m_swapchainExtent};
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);

    vkCmdEndRenderPass(commandBuffer);
  }

  VkImageSubresourceRange imageSubresourceRange{
      VK_IMAGE_ASPECT_COLOR_BIT, // aspectMask
      0,                         // baseMipLevel
      1,                         // levelCount
      0,                         // baseArrayLayer
      1,                         // layerCount
  };

  if (ctx().m_presentQueue != ctx().m_graphicsQueue) {
    VkImageMemoryBarrier barrierFromDrawToPresent{
        VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,   // sType
        nullptr,                                  // pNext
        VK_ACCESS_MEMORY_READ_BIT,                // srcAccessMask
        VK_ACCESS_MEMORY_READ_BIT,                // dstAccessMask
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, // oldLayout
        VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,          // newLayout
        ctx().m_graphicsQueueFamilyIndex,         // srcQueueFamilyIndex
        ctx().m_presentQueueFamilyIndex,          // dstQueueFamilyIndex
        m_swapchainImages[m_currentImageIndex],   // image
        imageSubresourceRange,                    // subresourceRange
    };

    vkCmdPipelineBarrier(
        commandBuffer,
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
        0,
        0,
        nullptr,
        0,
        nullptr,
        1,
        &barrierFromDrawToPresent);
  }

  VK_CHECK(vkEndCommandBuffer(commandBuffer));

  // Present
  VkPipelineStageFlags waitDstStageMask =
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

  VkSubmitInfo submitInfo = {
      VK_STRUCTURE_TYPE_SUBMIT_INFO, // sType
      nullptr,                       // pNext
      1,                             // waitSemaphoreCount
      &m_frameResources[m_currentFrame]
           .imageAvailableSemaphore, // pWaitSemaphores
      &waitDstStageMask,             // pWaitDstStageMask
      1,                             // commandBufferCount
      &commandBuffer,                // pCommandBuffers
      1,                             // signalSemaphoreCount
      &m_frameResources[m_currentFrame]
           .renderingFinishedSemaphore, // pSignalSemaphores
  };

  vkQueueSubmit(
      ctx().m_graphicsQueue,
      1,
      &submitInfo,
      m_frameResources[m_currentFrame].fence);

  VkPresentInfoKHR presentInfo = {
      VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
      nullptr, // pNext
      1,       // waitSemaphoreCount
      &m_frameResources[m_currentFrame]
           .renderingFinishedSemaphore, // pWaitSemaphores
      1,                                // swapchainCount
      &m_swapchain,                     // pSwapchains
      &m_currentImageIndex,             // pImageIndices
      nullptr,                          // pResults
  };

  VkResult result = vkQueuePresentKHR(ctx().m_presentQueue, &presentInfo);
  if (result == VK_ERROR_OUT_OF_DATE_KHR) {
    this->updateSize();
  } else {
    assert(result == VK_SUCCESS);
  }

  m_currentFrame = (m_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;

  auto timeAfter = std::chrono::high_resolution_clock::now();

  auto elapsedTime = std::chrono::duration_cast<std::chrono::microseconds>(
                         timeAfter - m_timeBefore)
                         .count();

  m_deltaTime = (double)elapsedTime / 1000000.0f;
}

void Window::beginRenderPass() {
  auto &commandBuffer = m_frameResources[m_currentFrame].commandBuffer;

  VkClearValue clearValues[2] = {};
  clearValues[0].color = {{
      this->clearColor.x,
      this->clearColor.y,
      this->clearColor.z,
      this->clearColor.w,
  }};
  clearValues[1].depthStencil = {1.0f, 0};

  VkRenderPassBeginInfo renderPassBeginInfo = {
      VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,     // sType
      nullptr,                                      // pNext
      m_renderPass,                                 // renderPass
      m_frameResources[m_currentFrame].framebuffer, // framebuffer
      {{0, 0}, m_swapchainExtent},                  // renderArea
      ARRAYSIZE(clearValues),                       // clearValueCount
      clearValues,                                  // pClearValues
  };

  vkCmdBeginRenderPass(
      commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

  VkViewport viewport{
      0.0f,                                         // x
      0.0f,                                         // y
      static_cast<float>(m_swapchainExtent.width),  // width
      static_cast<float>(m_swapchainExtent.height), // height
      0.0f,                                         // minDepth
      1.0f,                                         // maxDepth
  };

  vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

  VkRect2D scissor{{0, 0}, m_swapchainExtent};

  vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
}

void Window::endRenderPass() {
  auto &commandBuffer = m_frameResources[m_currentFrame].commandBuffer;

  this->imguiEndFrame();

  // End
  vkCmdEndRenderPass(commandBuffer);
}

void Window::updateSize() {
  this->destroyResizables();

  this->createSwapchain(this->getWidth(), this->getHeight());
  this->createSwapchainImageViews();
  this->createDepthStencilResources();
  this->createRenderPass();
  this->createImguiRenderPass();
  this->allocateGraphicsCommandBuffers();
}

uint32_t Window::getWidth() const {
  int width;
  SDL_GetWindowSize(m_window, &width, nullptr);
  return static_cast<uint32_t>(width);
}

uint32_t Window::getHeight() const {
  int height;
  SDL_GetWindowSize(m_window, nullptr, &height);
  return static_cast<uint32_t>(height);
}

bool Window::getRelativeMouse() const { return SDL_GetRelativeMouseMode(); }

void Window::setRelativeMouse(bool relative) {
  SDL_SetRelativeMouseMode((SDL_bool)relative);
}

void Window::getMouseState(int *x, int *y) const { SDL_GetMouseState(x, y); }

void Window::getRelativeMouseState(int *x, int *y) const {
  SDL_GetRelativeMouseState(x, y);
}

void Window::warpMouse(int x, int y) { SDL_WarpMouseInWindow(m_window, x, y); }

bool Window::isMouseLeftPressed() const {
  auto state = SDL_GetMouseState(nullptr, nullptr);
  return (state & SDL_BUTTON(SDL_BUTTON_LEFT));
}

bool Window::isMouseRightPressed() const {
  auto state = SDL_GetMouseState(nullptr, nullptr);
  return (state & SDL_BUTTON(SDL_BUTTON_RIGHT));
}

bool Window::isScancodePressed(Scancode scancode) const {
  const Uint8 *state = SDL_GetKeyboardState(nullptr);
  return (bool)state[(SDL_Scancode)scancode];
}

double Window::getDelta() const { return m_deltaTime; }

bool Window::getShouldClose() const { return m_shouldClose; }

void Window::setShouldClose(bool shouldClose) { m_shouldClose = shouldClose; }

VkSampleCountFlagBits Window::getMaxSampleCount() const {
  return m_maxSampleCount;
}

int Window::getCurrentFrameIndex() const { return m_currentFrame; }

VkCommandBuffer Window::getCurrentCommandBuffer() {
  return m_frameResources[m_currentFrame].commandBuffer;
}

void Window::imguiBeginFrame() {
  ImGui_ImplVulkan_NewFrame();
  ImGui_ImplSDL2_NewFrame(m_window);
  ImGui::NewFrame();
}

void Window::imguiEndFrame() { ImGui::Render(); }

void Window::createVulkanSurface() {
  if (!SDL_Vulkan_CreateSurface(m_window, ctx().m_instance, &m_surface)) {
    throw std::runtime_error(
        "Failed to create window surface: " + std::string(SDL_GetError()));
  }
}

void Window::createSyncObjects() {
  for (auto &resources : m_frameResources) {

    VkSemaphoreCreateInfo semaphoreCreateInfo = {};
    semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    semaphoreCreateInfo.pNext = nullptr;
    semaphoreCreateInfo.flags = 0;

    VK_CHECK(vkCreateSemaphore(
        ctx().m_device,
        &semaphoreCreateInfo,
        nullptr,
        &resources.imageAvailableSemaphore));

    VK_CHECK(vkCreateSemaphore(
        ctx().m_device,
        &semaphoreCreateInfo,
        nullptr,
        &resources.renderingFinishedSemaphore));

    VkFenceCreateInfo fenceCreateInfo = {};
    fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceCreateInfo.pNext = nullptr;
    fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    VK_CHECK(vkCreateFence(
        ctx().m_device, &fenceCreateInfo, nullptr, &resources.fence));
  }
}

void Window::createSwapchain(uint32_t width, uint32_t height) {
  for (const auto &imageView : m_swapchainImageViews) {
    if (imageView) {
      vkDestroyImageView(ctx().m_device, imageView, nullptr);
    }
  }

  VkSurfaceCapabilitiesKHR surfaceCapabilities;
  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
      ctx().m_physicalDevice, m_surface, &surfaceCapabilities);

  uint32_t count;

  vkGetPhysicalDeviceSurfaceFormatsKHR(
      ctx().m_physicalDevice, m_surface, &count, nullptr);
  fstl::fixed_vector<VkSurfaceFormatKHR> surfaceFormats(count);
  vkGetPhysicalDeviceSurfaceFormatsKHR(
      ctx().m_physicalDevice, m_surface, &count, surfaceFormats.data());

  vkGetPhysicalDeviceSurfacePresentModesKHR(
      ctx().m_physicalDevice, m_surface, &count, nullptr);
  fstl::fixed_vector<VkPresentModeKHR> presentModes(count);
  vkGetPhysicalDeviceSurfacePresentModesKHR(
      ctx().m_physicalDevice, m_surface, &count, presentModes.data());

  auto desiredNumImages = getSwapchainNumImages(surfaceCapabilities);
  auto desiredFormat = getSwapchainFormat(surfaceFormats);
  auto desiredExtent = getSwapchainExtent(width, height, surfaceCapabilities);
  auto desiredUsage = getSwapchainUsageFlags(surfaceCapabilities);
  auto desiredTransform = getSwapchainTransform(surfaceCapabilities);
  auto desiredPresentMode = getSwapchainPresentMode(presentModes);

  VkSwapchainKHR oldSwapchain = m_swapchain;

  VkSwapchainCreateInfoKHR createInfo{
      VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR, // sType
      nullptr,                                     // pNext
      0,                                           // flags
      m_surface,
      desiredNumImages,                  // minImageCount
      desiredFormat.format,              // imageFormat
      desiredFormat.colorSpace,          // imageColorSpace
      desiredExtent,                     // imageExtent
      1,                                 // imageArrayLayers
      desiredUsage,                      // imageUsage
      VK_SHARING_MODE_EXCLUSIVE,         // imageSharingMode
      0,                                 // queueFamilyIndexCount
      nullptr,                           // pQueueFamiylIndices
      desiredTransform,                  // preTransform
      VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR, // compositeAlpha
      desiredPresentMode,                // presentMode
      VK_TRUE,                           // clipped
      oldSwapchain                       // oldSwapchain
  };

  vkCreateSwapchainKHR(ctx().m_device, &createInfo, nullptr, &m_swapchain);

  if (oldSwapchain) {
    vkDestroySwapchainKHR(ctx().m_device, oldSwapchain, nullptr);
  }

  m_swapchainImageFormat = desiredFormat.format;
  m_swapchainExtent = desiredExtent;

  VK_CHECK(
      vkGetSwapchainImagesKHR(ctx().m_device, m_swapchain, &count, nullptr));
  m_swapchainImages.resize(count);
  VK_CHECK(vkGetSwapchainImagesKHR(
      ctx().m_device, m_swapchain, &count, m_swapchainImages.data()));
}

void Window::createSwapchainImageViews() {
  m_swapchainImageViews.resize(m_swapchainImages.size());

  for (size_t i = 0; i < m_swapchainImages.size(); i++) {
    VkImageViewCreateInfo createInfo{
        VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO, // sType
        nullptr,                                  // pNext
        0,                                        // flags
        m_swapchainImages[i],
        VK_IMAGE_VIEW_TYPE_2D,
        m_swapchainImageFormat,
        {
            VK_COMPONENT_SWIZZLE_IDENTITY, // r
            VK_COMPONENT_SWIZZLE_IDENTITY, // g
            VK_COMPONENT_SWIZZLE_IDENTITY, // b
            VK_COMPONENT_SWIZZLE_IDENTITY, // a
        },
        {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1}};

    VK_CHECK(vkCreateImageView(
        ctx().m_device, &createInfo, nullptr, &m_swapchainImageViews[i]));
  }
}

void Window::allocateGraphicsCommandBuffers() {
  VkCommandBufferAllocateInfo allocateInfo = {};
  allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocateInfo.pNext = nullptr;
  allocateInfo.commandPool = ctx().m_graphicsCommandPool;
  allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocateInfo.commandBufferCount = MAX_FRAMES_IN_FLIGHT;

  fstl::fixed_vector<VkCommandBuffer> commandBuffers(MAX_FRAMES_IN_FLIGHT);

  vkAllocateCommandBuffers(
      ctx().m_device, &allocateInfo, commandBuffers.data());

  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    m_frameResources[i].commandBuffer = commandBuffers[i];
  }
}

void Window::createDepthStencilResources() {
  assert(ctx().getSupportedDepthFormat(&m_depthImageFormat));

  VkImageCreateInfo imageCreateInfo = {
      VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO, // sType
      nullptr,                             // pNext
      0,                                   // flags
      VK_IMAGE_TYPE_2D,
      m_depthImageFormat,
      {
          m_swapchainExtent.width,
          m_swapchainExtent.height,
          1,
      },
      1,
      1,
      VK_SAMPLE_COUNT_1_BIT,
      VK_IMAGE_TILING_OPTIMAL,
      VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
      VK_SHARING_MODE_EXCLUSIVE,
      0,
      nullptr,
      VK_IMAGE_LAYOUT_UNDEFINED,

  };

  VmaAllocationCreateInfo allocInfo{};
  allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

  VK_CHECK(vmaCreateImage(
      ctx().m_allocator,
      &imageCreateInfo,
      &allocInfo,
      &m_depthStencil.image,
      &m_depthStencil.allocation,
      nullptr));

  VkImageViewCreateInfo imageViewCreateInfo = {
      VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO, // sType
      nullptr,                                  // pNext
      0,                                        // flags
      m_depthStencil.image,
      VK_IMAGE_VIEW_TYPE_2D,
      m_depthImageFormat,
      {
          VK_COMPONENT_SWIZZLE_IDENTITY, // r
          VK_COMPONENT_SWIZZLE_IDENTITY, // g
          VK_COMPONENT_SWIZZLE_IDENTITY, // b
          VK_COMPONENT_SWIZZLE_IDENTITY, // a
      },                                 // components
      {
          VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT, // aspectMask
          0, // baseMipLevel
          1, // levelCount
          0, // baseArrayLayer
          1, // layerCount
      },     // subresourceRange
  };

  VK_CHECK(vkCreateImageView(
      ctx().m_device, &imageViewCreateInfo, nullptr, &m_depthStencil.view));
}

void Window::createRenderPass() {
  VkAttachmentDescription attachmentDescriptions[] = {
      // Resolved color attachment
      VkAttachmentDescription{
          0,                                // flags
          m_swapchainImageFormat,           // format
          VK_SAMPLE_COUNT_1_BIT,            // samples
          VK_ATTACHMENT_LOAD_OP_CLEAR,      // loadOp
          VK_ATTACHMENT_STORE_OP_STORE,     // storeOp
          VK_ATTACHMENT_LOAD_OP_DONT_CARE,  // stencilLoadOp
          VK_ATTACHMENT_STORE_OP_DONT_CARE, // stencilStoreOp
          VK_IMAGE_LAYOUT_UNDEFINED,        // initialLayout
          VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,  // finalLayout
      },

      // Resolved depth attachment
      VkAttachmentDescription{
          0,                                                // flags
          m_depthImageFormat,                               // format
          VK_SAMPLE_COUNT_1_BIT,                            // samples
          VK_ATTACHMENT_LOAD_OP_CLEAR,                      // loadOp
          VK_ATTACHMENT_STORE_OP_STORE,                     // storeOp
          VK_ATTACHMENT_LOAD_OP_DONT_CARE,                  // stencilLoadOp
          VK_ATTACHMENT_STORE_OP_DONT_CARE,                 // stencilStoreOp
          VK_IMAGE_LAYOUT_UNDEFINED,                        // initialLayout
          VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, // finalLayout
      },
  };

  VkAttachmentReference colorAttachmentReference = {
      0,                                        // attachment
      VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, // layout
  };

  VkAttachmentReference depthAttachmentReference = {
      1,                                                // attachment
      VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, // layout
  };

  VkSubpassDescription subpassDescription = {
      {},                              // flags
      VK_PIPELINE_BIND_POINT_GRAPHICS, // pipelineBindPoint
      0,                               // inputAttachmentCount
      nullptr,                         // pInputAttachments
      1,                               // colorAttachmentCount
      &colorAttachmentReference,       // pColorAttachments
      nullptr,                         // pResolveAttachments
      &depthAttachmentReference,       // pDepthStencilAttachment
      0,                               // preserveAttachmentCount
      nullptr,                         // pPreserveAttachments
  };

  VkSubpassDependency dependencies[] = {
      VkSubpassDependency{
          VK_SUBPASS_EXTERNAL,                           // srcSubpass
          0,                                             // dstSubpass
          VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,          // srcStageMask
          VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, // dstStageMask
          VK_ACCESS_MEMORY_READ_BIT,                     // srcAccessMask
          VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
              VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, // dstAccessMask
          VK_DEPENDENCY_BY_REGION_BIT,              // dependencyFlags
      },
      VkSubpassDependency{
          0,                                             // srcSubpass
          VK_SUBPASS_EXTERNAL,                           // dstSubpass
          VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, // srcStageMask
          VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,          // dstStageMask
          VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
              VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, // srcAccessMask
          VK_ACCESS_MEMORY_READ_BIT,                // dstAccessMask
          VK_DEPENDENCY_BY_REGION_BIT,              // dependencyFlags
      },
  };

  VkRenderPassCreateInfo renderPassCreateInfo = {
      VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO, // sType
      nullptr,                                   // pNext
      0,                                         // flags
      static_cast<uint32_t>(
          ARRAYSIZE(attachmentDescriptions)),         // attachmentCount
      attachmentDescriptions,                         // pAttachments
      1,                                              // subpassCount
      &subpassDescription,                            // pSubpasses
      static_cast<uint32_t>(ARRAYSIZE(dependencies)), // dependencyCount
      dependencies,                                   // pDependencies
  };

  VK_CHECK(vkCreateRenderPass(
      ctx().m_device, &renderPassCreateInfo, nullptr, &m_renderPass));
}

void Window::createImguiRenderPass() {
  VkAttachmentDescription attachment = {};
  attachment.format = m_swapchainImageFormat;
  attachment.samples = VK_SAMPLE_COUNT_1_BIT;
  attachment.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

  VkAttachmentReference color_attachment = {};
  color_attachment.attachment = 0;
  color_attachment.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  VkSubpassDescription subpass = {};
  subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpass.colorAttachmentCount = 1;
  subpass.pColorAttachments = &color_attachment;

  VkSubpassDependency dependency = {};
  dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
  dependency.dstSubpass = 0;
  dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependency.srcAccessMask = 0;
  dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

  VkRenderPassCreateInfo renderPassCreateInfo = {
      VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
      nullptr,     // pNext
      0,           // flags
      1,           // attachmentCount
      &attachment, // pAttachments
      1,           // subpassCount
      &subpass,    // pSubpasses
      1,           // dependencyCount
      &dependency, // pDependencies
  };

  VK_CHECK(vkCreateRenderPass(
      ctx().m_device, &renderPassCreateInfo, nullptr, &m_imguiRenderPass));
}

void Window::initImgui() {
  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
  (void)io;

  ImGui_ImplSDL2_InitForVulkan(m_window);

  // Setup Vulkan binding
  ImGui_ImplVulkan_InitInfo init_info = {};
  init_info.Instance = ctx().m_instance;
  init_info.PhysicalDevice = ctx().m_physicalDevice;
  init_info.Device = ctx().m_device;
  init_info.QueueFamily = ctx().m_graphicsQueueFamilyIndex;
  init_info.Queue = ctx().m_graphicsQueue;
  init_info.PipelineCache = VK_NULL_HANDLE;
  init_info.DescriptorPool = *ctx().m_descriptorManager.getPool(DESC_IMGUI);
  init_info.Allocator = nullptr;
  init_info.CheckVkResultFn = [](VkResult result) {
    if (result != VK_SUCCESS) {
      throw std::runtime_error("Failed to initialize IMGUI!");
    }
  };
  ImGui_ImplVulkan_Init(&init_info, m_imguiRenderPass);

  // Setup style
  ImGui::StyleColorsDark();

  // Upload Fonts
  {
    // Use any command queue
    VkCommandPool commandPool = ctx().m_graphicsCommandPool;
    VkCommandBuffer commandBuffer =
        m_frameResources[m_currentFrame].commandBuffer;

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

void Window::regenFramebuffer(
    VkFramebuffer &framebuffer, VkImageView &swapchainImageView) {
  vkDestroyFramebuffer(ctx().m_device, framebuffer, nullptr);

  VkImageView attachments[]{
      swapchainImageView,
      m_depthStencil.view,
  };

  VkFramebufferCreateInfo createInfo = {
      VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,     // sType
      nullptr,                                       // pNext
      0,                                             // flags
      m_renderPass,                                  // renderPass
      static_cast<uint32_t>(ARRAYSIZE(attachments)), // attachmentCount
      attachments,                                   // pAttachments
      m_swapchainExtent.width,                       // width
      m_swapchainExtent.height,                      // height
      1,                                             // layers
  };

  VK_CHECK(
      vkCreateFramebuffer(ctx().m_device, &createInfo, nullptr, &framebuffer));
}

void Window::regenImguiFramebuffer(
    VkFramebuffer &framebuffer, VkImageView &swapchainImageView) {
  vkDestroyFramebuffer(ctx().m_device, framebuffer, nullptr);

  VkFramebufferCreateInfo createInfo = {
      VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO, // sType
      nullptr,                                   // pNext
      0,                                         // flags
      m_imguiRenderPass,                         // renderPass
      1,                                         // attachmentCount
      &swapchainImageView,                       // pAttachments
      m_swapchainExtent.width,                   // width
      m_swapchainExtent.height,                  // height
      1,                                         // layers
  };

  VK_CHECK(
      vkCreateFramebuffer(ctx().m_device, &createInfo, nullptr, &framebuffer));
}

void Window::destroyResizables() {
  VK_CHECK(vkDeviceWaitIdle(ctx().m_device));

  for (auto &resources : m_frameResources) {
    vkFreeCommandBuffers(
        ctx().m_device,
        ctx().m_graphicsCommandPool,
        1,
        &resources.commandBuffer);
  }

  if (m_depthStencil.image) {
    vkDestroyImageView(ctx().m_device, m_depthStencil.view, nullptr);
    vmaDestroyImage(
        ctx().m_allocator, m_depthStencil.image, m_depthStencil.allocation);
    m_depthStencil.image = nullptr;
    m_depthStencil.allocation = VK_NULL_HANDLE;
  }

  vkDestroyRenderPass(ctx().m_device, m_imguiRenderPass, nullptr);
  vkDestroyRenderPass(ctx().m_device, m_renderPass, nullptr);
}

uint32_t Window::getSwapchainNumImages(
    const VkSurfaceCapabilitiesKHR &surfaceCapabilities) {
  uint32_t imageCount = surfaceCapabilities.minImageCount + 1;

  if (surfaceCapabilities.maxImageCount > 0 &&
      imageCount > surfaceCapabilities.maxImageCount) {
    imageCount = surfaceCapabilities.maxImageCount;
  }

  return imageCount;
}

VkSurfaceFormatKHR Window::getSwapchainFormat(
    const fstl::fixed_vector<VkSurfaceFormatKHR> &formats) {
  if (formats.size() == 1 && formats[0].format == VK_FORMAT_UNDEFINED) {
    return {VK_FORMAT_R8G8B8A8_UNORM, VK_COLORSPACE_SRGB_NONLINEAR_KHR};
  }

  for (const auto &format : formats) {
    if (format.format == VK_FORMAT_R8G8B8A8_UNORM) {
      return format;
    }
  }

  return formats[0];
}

VkExtent2D Window::getSwapchainExtent(
    uint32_t width,
    uint32_t height,
    const VkSurfaceCapabilitiesKHR &surfaceCapabilities) {
  if (surfaceCapabilities.currentExtent.width == static_cast<uint32_t>(-1)) {
    VkExtent2D swapchainExtent = {width, height};
    if (swapchainExtent.width < surfaceCapabilities.minImageExtent.width) {
      swapchainExtent.width = surfaceCapabilities.minImageExtent.width;
    }

    if (swapchainExtent.height < surfaceCapabilities.minImageExtent.height) {
      swapchainExtent.height = surfaceCapabilities.minImageExtent.height;
    }

    if (swapchainExtent.width > surfaceCapabilities.maxImageExtent.width) {
      swapchainExtent.width = surfaceCapabilities.maxImageExtent.width;
    }

    if (swapchainExtent.height > surfaceCapabilities.maxImageExtent.height) {
      swapchainExtent.height = surfaceCapabilities.maxImageExtent.height;
    }

    return swapchainExtent;
  }

  return surfaceCapabilities.currentExtent;
}

VkImageUsageFlags Window::getSwapchainUsageFlags(
    const VkSurfaceCapabilitiesKHR &surfaceCapabilities) {
  if (surfaceCapabilities.supportedUsageFlags &
      VK_IMAGE_USAGE_TRANSFER_DST_BIT) {
    return VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
           VK_IMAGE_USAGE_TRANSFER_DST_BIT;
  }

  fstl::log::fatal(
      "VK_IMAGE_USAGE_TRANSFER_DST image usage is not supported by the "
      "swapchain!\n"
      "Supported swapchain image usages include:\n"
      "{}\n{}\n{}\n{}\n{}\n{}\n{}\n{}\n",
      (surfaceCapabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_SRC_BIT
           ? "    VK_IMAGE_USAGE_TRANSFER_SRC\n"
           : ""),
      (surfaceCapabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_DST_BIT
           ? "    VK_IMAGE_USAGE_TRANSFER_DST\n"
           : ""),
      (surfaceCapabilities.supportedUsageFlags & VK_IMAGE_USAGE_SAMPLED_BIT
           ? "    VK_IMAGE_USAGE_SAMPLED\n"
           : ""),
      (surfaceCapabilities.supportedUsageFlags & VK_IMAGE_USAGE_STORAGE_BIT
           ? "    VK_IMAGE_USAGE_STORAGE\n"
           : ""),
      (surfaceCapabilities.supportedUsageFlags &
               VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT
           ? "    VK_IMAGE_USAGE_COLOR_ATTACHMENT\n"
           : ""),
      (surfaceCapabilities.supportedUsageFlags &
               VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT
           ? "    VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT\n"
           : ""),
      (surfaceCapabilities.supportedUsageFlags &
               VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT
           ? "    VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT\n"
           : ""),
      (surfaceCapabilities.supportedUsageFlags &
               VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT
           ? "    VK_IMAGE_USAGE_INPUT_ATTACHMENT"
           : ""));

  return static_cast<VkImageUsageFlags>(-1);
}

VkSurfaceTransformFlagBitsKHR Window::getSwapchainTransform(
    const VkSurfaceCapabilitiesKHR &surfaceCapabilities) {
  if (surfaceCapabilities.supportedTransforms &
      VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR) {
    return VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
  } else {
    return surfaceCapabilities.currentTransform;
  }
}

VkPresentModeKHR Window::getSwapchainPresentMode(
    const fstl::fixed_vector<VkPresentModeKHR> &presentModes) {
  for (const auto &presentMode : presentModes) {
    if (presentMode == VK_PRESENT_MODE_IMMEDIATE_KHR) {
      fstl::log::debug("Recreating swapchain using immediate present mode");
      return presentMode;
    }
  }

  for (const auto &presentMode : presentModes) {
    if (presentMode == VK_PRESENT_MODE_FIFO_KHR) {
      fstl::log::debug("Recreating swapchain using FIFO present mode");
      return presentMode;
    }
  }

  for (const auto &presentMode : presentModes) {
    if (presentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
      fstl::log::debug("Recreating swapchain using mailbox present mode");
      return presentMode;
    }
  }

  fstl::log::fatal("FIFO present mode is not supported by the swapchain!");

  return static_cast<VkPresentModeKHR>(-1);
}
