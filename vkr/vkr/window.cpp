#include "window.hpp"
#include "context.hpp"
#include "util.hpp"
#include <SDL2/SDL_vulkan.h>
#include <fstl/logging.hpp>
#include <imgui/imgui.h>
#include <imgui/imgui_impl_sdl.h>
#include <imgui/imgui_impl_vulkan.h>

using namespace vkr;

Window::Window(const char *title, uint32_t width, uint32_t height) {
  auto subsystems = SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_TIMER;
  if (subsystems == SDL_WasInit(0)) {
    SDL_Init(subsystems);
  }

  this->window_ = SDL_CreateWindow(
      title,
      SDL_WINDOWPOS_CENTERED,
      SDL_WINDOWPOS_CENTERED,
      width,
      height,
      SDL_WINDOW_SHOWN | SDL_WINDOW_VULKAN);

  if (this->window_ == nullptr) {
    throw std::runtime_error("Failed to create SDL window");
  }

  SDL_SetWindowResizable(this->window_, SDL_TRUE);

  uint32_t sdlExtensionCount = 0;
  SDL_Vulkan_GetInstanceExtensions(this->window_, &sdlExtensionCount, nullptr);
  std::vector<const char *> sdlExtensions(sdlExtensionCount);
  SDL_Vulkan_GetInstanceExtensions(
      this->window_, &sdlExtensionCount, sdlExtensions.data());

  // These context initialization functions only run if the context is
  // uninitialized
  ctx::preInitialize(sdlExtensions);

  this->createVulkanSurface();

  // Lazily create vulkan context stuff
  ctx::lateInitialize(this->surface_);

  VkBool32 supported;
  vkGetPhysicalDeviceSurfaceSupportKHR(
      ctx::physicalDevice,
      ctx::presentQueueFamilyIndex,
      this->surface_,
      &supported);
  if (!supported) {
    throw std::runtime_error(
        "Selected present queue does not support this window's surface");
  }

  this->maxMsaaSamples_ = ctx::getMaxUsableSampleCount();

  this->createSyncObjects();

  this->createSwapchain(this->getWidth(), this->getHeight());
  this->createSwapchainImageViews();

  this->allocateGraphicsCommandBuffers();

  this->createDepthStencilResources();

  this->createMultisampleTargets();

  this->createRenderPass();

  this->createImguiRenderPass();

  this->initImgui();
}

void Window::destroy() {
  VK_CHECK(vkDeviceWaitIdle(ctx::device));

  ImGui_ImplVulkan_Shutdown();
  ImGui_ImplSDL2_Shutdown();
  ImGui::DestroyContext();

  this->destroyResizables();

  for (auto &swapchainImageView : this->swapchainImageViews_) {
    vkDestroyImageView(ctx::device, swapchainImageView, nullptr);
  }

  vkDestroySwapchainKHR(ctx::device, this->swapchain_, nullptr);

  for (auto &frameResource : this->frameResources_) {
    vkDestroyFramebuffer(ctx::device, frameResource.framebuffer, nullptr);
    vkDestroyFramebuffer(ctx::device, frameResource.imguiFramebuffer, nullptr);
    vkDestroySemaphore(
        ctx::device, frameResource.imageAvailableSemaphore, nullptr);
    vkDestroySemaphore(
        ctx::device, frameResource.renderingFinishedSemaphore, nullptr);
    vkDestroyFence(ctx::device, frameResource.fence, nullptr);
  }

  vkDestroySurfaceKHR(ctx::instance, this->surface_, nullptr);

  SDL_DestroyWindow(this->window_);
}

SDL_Event Window::pollEvent() {
  SDL_Event event;
  SDL_PollEvent(&event);

  ImGui_ImplSDL2_ProcessEvent(&event);

  return event;
}

void Window::present(std::function<void()> drawFunction) {
  this->lastTicks_ = SDL_GetTicks();

  // Begin
  vkWaitForFences(
      ctx::device,
      1,
      &this->frameResources_[this->currentFrame_].fence,
      VK_TRUE,
      UINT64_MAX);

  vkResetFences(
      ctx::device, 1, &this->frameResources_[this->currentFrame_].fence);

  if (vkAcquireNextImageKHR(
          ctx::device,
          this->swapchain_,
          UINT64_MAX,
          this->frameResources_[this->currentFrame_].imageAvailableSemaphore,
          VK_NULL_HANDLE,
          &this->currentImageIndex_) == VK_ERROR_OUT_OF_DATE_KHR) {
    this->updateSize();
  }

  VkImageSubresourceRange imageSubresourceRange{
      VK_IMAGE_ASPECT_COLOR_BIT, // aspectMask
      0,                         // baseMipLevel
      1,                         // levelCount
      0,                         // baseArrayLayer
      1,                         // layerCount
  };

  this->regenFramebuffer(
      this->frameResources_[this->currentFrame_].framebuffer,
      this->swapchainImageViews_[this->currentImageIndex_]);

  this->regenImguiFramebuffer(
      this->frameResources_[this->currentFrame_].imguiFramebuffer,
      this->swapchainImageViews_[this->currentImageIndex_]);

  VkCommandBufferBeginInfo beginInfo = {};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
  beginInfo.pInheritanceInfo = nullptr;

  auto &commandBuffer =
      this->frameResources_[this->currentFrame_].commandBuffer;

  VK_CHECK(vkBeginCommandBuffer(commandBuffer, &beginInfo));

  if (ctx::presentQueue != ctx::graphicsQueue) {
    VkImageMemoryBarrier barrierFromPresentToDraw = {
        VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,           // sType
        nullptr,                                          // pNext
        VK_ACCESS_MEMORY_READ_BIT,                        // srcAccessMask
        VK_ACCESS_MEMORY_READ_BIT,                        // dstAccessMask
        VK_IMAGE_LAYOUT_UNDEFINED,                        // oldLayout
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,         // newLayout
        ctx::presentQueueFamilyIndex,                     // srcQueueFamilyIndex
        ctx::graphicsQueueFamilyIndex,                    // dstQueueFamilyIndex
        this->swapchainImages_[this->currentImageIndex_], // image
        imageSubresourceRange,                            // subresourceRange
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

  VkClearValue clearColor = {{{
      this->clearColor.x,
      this->clearColor.y,
      this->clearColor.z,
      this->clearColor.w,
  }}};

  std::array<VkClearValue, 4> clearValues{
      VkClearValue{},
      clearColor,
      VkClearValue{},
      VkClearValue{{{1.0f, 0}}},
  };

  if (this->msaaSamples_ != VK_SAMPLE_COUNT_1_BIT) {
    clearValues = {
        clearColor,
        clearColor,
        VkClearValue{{{1.0f, 0}}},
    };
  }

  VkRenderPassBeginInfo renderPassBeginInfo = {
      VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,               // sType
      nullptr,                                                // pNext
      this->renderPass_,                                      // renderPass
      this->frameResources_[this->currentFrame_].framebuffer, // framebuffer
      {{0, 0}, this->swapchainExtent_},                       // renderArea
      static_cast<uint32_t>(clearValues.size()),              // clearValueCount
      clearValues.data(),                                     // pClearValues
  };

  vkCmdBeginRenderPass(
      commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

  VkViewport viewport{
      0.0f,                                              // x
      0.0f,                                              // y
      static_cast<float>(this->swapchainExtent_.width),  // width
      static_cast<float>(this->swapchainExtent_.height), // height
      0.0f,                                              // minDepth
      1.0f,                                              // maxDepth
  };

  vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

  VkRect2D scissor{{0, 0}, this->swapchainExtent_};

  vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

  this->imguiBeginFrame();

  // Draw
  drawFunction();

  this->imguiEndFrame();

  // End
  vkCmdEndRenderPass(commandBuffer);

  {
    VkRenderPassBeginInfo imguiRenderPassBeginInfo = {
        VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO, // sType
        nullptr,                                  // pNext
        this->imguiRenderPass_,                   // renderPass
        this->frameResources_[this->currentFrame_]
            .imguiFramebuffer,            // framebuffer
        {{0, 0}, this->swapchainExtent_}, // renderArea
        0,                                // clearValueCount
        nullptr,                          // pClearValues
    };

    vkCmdBeginRenderPass(
        commandBuffer, &imguiRenderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

    VkViewport viewport{
        0.0f,                                              // x
        0.0f,                                              // y
        static_cast<float>(this->swapchainExtent_.width),  // width
        static_cast<float>(this->swapchainExtent_.height), // height
        0.0f,                                              // minDepth
        1.0f,                                              // maxDepth
    };

    VkRect2D scissor{{0, 0}, this->swapchainExtent_};
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);

    vkCmdEndRenderPass(commandBuffer);
  }

  if (ctx::presentQueue != ctx::graphicsQueue) {
    VkImageMemoryBarrier barrierFromDrawToPresent{
        VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,           // sType
        nullptr,                                          // pNext
        VK_ACCESS_MEMORY_READ_BIT,                        // srcAccessMask
        VK_ACCESS_MEMORY_READ_BIT,                        // dstAccessMask
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,         // oldLayout
        VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,                  // newLayout
        ctx::graphicsQueueFamilyIndex,                    // srcQueueFamilyIndex
        ctx::presentQueueFamilyIndex,                     // dstQueueFamilyIndex
        this->swapchainImages_[this->currentImageIndex_], // image
        imageSubresourceRange,                            // subresourceRange
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
      &this->frameResources_[this->currentFrame_]
           .imageAvailableSemaphore, // pWaitSemaphores
      &waitDstStageMask,             // pWaitDstStageMask
      1,                             // commandBufferCount
      &commandBuffer,                // pCommandBuffers
      1,                             // signalSemaphoreCount
      &this->frameResources_[this->currentFrame_]
           .renderingFinishedSemaphore, // pSignalSemaphores
  };

  vkQueueSubmit(
      ctx::graphicsQueue,
      1,
      &submitInfo,
      this->frameResources_[this->currentFrame_].fence);

  VkPresentInfoKHR presentInfo = {
      VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
      nullptr, // pNext
      1,       // waitSemaphoreCount
      &this->frameResources_[this->currentFrame_]
           .renderingFinishedSemaphore, // pWaitSemaphores
      1,                                // swapchainCount
      &this->swapchain_,                // pSwapchains
      &this->currentImageIndex_,        // pImageIndices
      nullptr,                          // pResults
  };

  VkResult result = vkQueuePresentKHR(ctx::presentQueue, &presentInfo);
  if (result == VK_ERROR_OUT_OF_DATE_KHR) {
    this->updateSize();
  } else {
    assert(result == VK_SUCCESS);
  }

  this->currentFrame_ = (this->currentFrame_ + 1) % MAX_FRAMES_IN_FLIGHT;

  this->deltaTicks_ = SDL_GetTicks() - this->lastTicks_;
}

void Window::updateSize() {
  this->destroyResizables();

  this->createSwapchain(this->getWidth(), this->getHeight());
  this->createSwapchainImageViews();
  this->createDepthStencilResources();
  this->createMultisampleTargets();
  this->createRenderPass();
  this->createImguiRenderPass();
  this->allocateGraphicsCommandBuffers();
}

uint32_t Window::getWidth() const {
  int width;
  SDL_GetWindowSize(this->window_, &width, nullptr);
  return static_cast<uint32_t>(width);
}

uint32_t Window::getHeight() const {
  int height;
  SDL_GetWindowSize(this->window_, nullptr, &height);
  return static_cast<uint32_t>(height);
}

bool Window::getRelativeMouse() const { return SDL_GetRelativeMouseMode(); }

void Window::setRelativeMouse(bool relative) {
  SDL_SetRelativeMouseMode((SDL_bool)relative);
}

int Window::getMouseX() const {
  int x;
  SDL_GetMouseState(&x, nullptr);
  return x;
}

int Window::getMouseY() const {
  int y;
  SDL_GetMouseState(nullptr, &y);
  return y;
}

int Window::getRelativeMouseX() const {
  int x;
  SDL_GetRelativeMouseState(&x, nullptr);
  return x;
}

int Window::getRelativeMouseY() const {
  int y;
  SDL_GetRelativeMouseState(nullptr, &y);
  return y;
}

float Window::getDelta() const { return (float)this->deltaTicks_ / 1000.0f; }

bool Window::getShouldClose() const { return this->shouldClose_; }

void Window::setShouldClose(bool shouldClose) {
  this->shouldClose_ = shouldClose;
}

VkSampleCountFlagBits Window::getMaxMSAASamples() const {
  return this->maxMsaaSamples_;
}

VkSampleCountFlagBits Window::getMSAASamples() const {
  return this->msaaSamples_;
}

void Window::setMSAASamples(VkSampleCountFlagBits sampleCount) {
  if (sampleCount <= this->maxMsaaSamples_) {
    this->msaaSamples_ = sampleCount;

    // Recreate stuff using new sample count
    this->updateSize();
  } else {
    throw std::runtime_error("Invalid MSAA sample count");
  }
}

int Window::getCurrentFrameIndex() const { return this->currentFrame_; }

VkCommandBuffer Window::getCurrentCommandBuffer() {
  return this->frameResources_[this->currentFrame_].commandBuffer;
}

void Window::imguiBeginFrame() {
  ImGui_ImplVulkan_NewFrame();
  ImGui_ImplSDL2_NewFrame(window_);
  ImGui::NewFrame();
}

void Window::imguiEndFrame() { ImGui::Render(); }

void Window::createVulkanSurface() {
  if (!SDL_Vulkan_CreateSurface(
          this->window_, ctx::instance, &this->surface_)) {
    throw std::runtime_error(
        "Failed to create window surface: " + std::string(SDL_GetError()));
  }
}

void Window::createSyncObjects() {
  for (auto &resources : this->frameResources_) {

    VkSemaphoreCreateInfo semaphoreCreateInfo = {};
    semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    semaphoreCreateInfo.pNext = nullptr;
    semaphoreCreateInfo.flags = 0;

    VK_CHECK(vkCreateSemaphore(
        ctx::device,
        &semaphoreCreateInfo,
        nullptr,
        &resources.imageAvailableSemaphore));

    VK_CHECK(vkCreateSemaphore(
        ctx::device,
        &semaphoreCreateInfo,
        nullptr,
        &resources.renderingFinishedSemaphore));

    VkFenceCreateInfo fenceCreateInfo = {};
    fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceCreateInfo.pNext = nullptr;
    fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    VK_CHECK(vkCreateFence(
        ctx::device, &fenceCreateInfo, nullptr, &resources.fence));
  }
}

void Window::createSwapchain(uint32_t width, uint32_t height) {
  for (const auto &imageView : this->swapchainImageViews_) {
    if (imageView) {
      vkDestroyImageView(ctx::device, imageView, nullptr);
    }
  }
  this->swapchainImageViews_.clear();

  VkSurfaceCapabilitiesKHR surfaceCapabilities;
  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
      ctx::physicalDevice, this->surface_, &surfaceCapabilities);

  uint32_t count;

  vkGetPhysicalDeviceSurfaceFormatsKHR(
      ctx::physicalDevice, this->surface_, &count, nullptr);
  fstl::fixed_vector<VkSurfaceFormatKHR> surfaceFormats(count);
  vkGetPhysicalDeviceSurfaceFormatsKHR(
      ctx::physicalDevice, this->surface_, &count, surfaceFormats.data());

  vkGetPhysicalDeviceSurfacePresentModesKHR(
      ctx::physicalDevice, this->surface_, &count, nullptr);
  fstl::fixed_vector<VkPresentModeKHR> presentModes(count);
  vkGetPhysicalDeviceSurfacePresentModesKHR(
      ctx::physicalDevice, this->surface_, &count, presentModes.data());

  auto desiredNumImages = getSwapchainNumImages(surfaceCapabilities);
  auto desiredFormat = getSwapchainFormat(surfaceFormats);
  auto desiredExtent = getSwapchainExtent(width, height, surfaceCapabilities);
  auto desiredUsage = getSwapchainUsageFlags(surfaceCapabilities);
  auto desiredTransform = getSwapchainTransform(surfaceCapabilities);
  auto desiredPresentMode = getSwapchainPresentMode(presentModes);

  VkSwapchainKHR oldSwapchain = this->swapchain_;

  VkSwapchainCreateInfoKHR createInfo{
      VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR, // sType
      nullptr,                                     // pNext
      0,                                           // flags
      this->surface_,
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

  vkCreateSwapchainKHR(ctx::device, &createInfo, nullptr, &this->swapchain_);

  if (oldSwapchain) {
    vkDestroySwapchainKHR(ctx::device, oldSwapchain, nullptr);
  }

  this->swapchainImageFormat_ = desiredFormat.format;
  this->swapchainExtent_ = desiredExtent;

  VK_CHECK(
      vkGetSwapchainImagesKHR(ctx::device, this->swapchain_, &count, nullptr));
  this->swapchainImages_.resize(count);
  VK_CHECK(vkGetSwapchainImagesKHR(
      ctx::device, this->swapchain_, &count, this->swapchainImages_.data()));
}

void Window::createSwapchainImageViews() {
  this->swapchainImageViews_.resize(this->swapchainImages_.size());

  for (size_t i = 0; i < swapchainImages_.size(); i++) {
    VkImageViewCreateInfo createInfo{
        VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO, // sType
        nullptr,                                  // pNext
        0,                                        // flags
        this->swapchainImages_[i],
        VK_IMAGE_VIEW_TYPE_2D,
        this->swapchainImageFormat_,
        {
            VK_COMPONENT_SWIZZLE_IDENTITY, // r
            VK_COMPONENT_SWIZZLE_IDENTITY, // g
            VK_COMPONENT_SWIZZLE_IDENTITY, // b
            VK_COMPONENT_SWIZZLE_IDENTITY, // a
        },
        {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1}};

    VK_CHECK(vkCreateImageView(
        ctx::device, &createInfo, nullptr, &this->swapchainImageViews_[i]));
  }
}

void Window::allocateGraphicsCommandBuffers() {
  VkCommandBufferAllocateInfo allocateInfo = {};
  allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocateInfo.pNext = nullptr;
  allocateInfo.commandPool = ctx::graphicsCommandPool;
  allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocateInfo.commandBufferCount = MAX_FRAMES_IN_FLIGHT;

  fstl::fixed_vector<VkCommandBuffer> commandBuffers(MAX_FRAMES_IN_FLIGHT);

  vkAllocateCommandBuffers(ctx::device, &allocateInfo, commandBuffers.data());

  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    this->frameResources_[i].commandBuffer = commandBuffers[i];
  }
}

void Window::createDepthStencilResources() {
  VkFormat depthFormats[5] = {VK_FORMAT_D32_SFLOAT_S8_UINT,
                              VK_FORMAT_D32_SFLOAT,
                              VK_FORMAT_D24_UNORM_S8_UINT,
                              VK_FORMAT_D16_UNORM_S8_UINT,
                              VK_FORMAT_D16_UNORM};

  bool validDepthFormat = false;
  for (auto &format : depthFormats) {
    VkFormatProperties formatProps;
    vkGetPhysicalDeviceFormatProperties(
        ctx::physicalDevice, format, &formatProps);

    if (formatProps.optimalTilingFeatures &
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) {
      this->depthImageFormat_ = format;
      validDepthFormat = true;
      break;
    }
  }
  assert(validDepthFormat);

  VkImageCreateInfo imageCreateInfo = {
      VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO, // sType
      nullptr,                             // pNext
      0,                                   // flags
      VK_IMAGE_TYPE_2D,
      this->depthImageFormat_,
      {
          this->swapchainExtent_.width,
          this->swapchainExtent_.height,
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
      ctx::allocator,
      &imageCreateInfo,
      &allocInfo,
      &this->depthStencil_.image,
      &this->depthStencil_.allocation,
      nullptr));

  VkImageViewCreateInfo imageViewCreateInfo = {
      VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO, // sType
      nullptr,                                  // pNext
      0,                                        // flags
      this->depthStencil_.image,
      VK_IMAGE_VIEW_TYPE_2D,
      this->depthImageFormat_,
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
      ctx::device, &imageViewCreateInfo, nullptr, &this->depthStencil_.view));
}

void Window::createMultisampleTargets() {
  // Color target
  {
    VkImageCreateInfo imageCreateInfo = {
        VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO, // sType
        nullptr,                             // pNext
        0,                                   // flags
        VK_IMAGE_TYPE_2D,                    // imageType
        this->swapchainImageFormat_,         // format
        {
            this->swapchainExtent_.width,  // width
            this->swapchainExtent_.height, // height
            1,                             // depth
        },                                 // extent
        1,                                 // mipLevels
        1,                                 // arrayLayers
        this->msaaSamples_,                // samples
        VK_IMAGE_TILING_OPTIMAL,           // tiling
        VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT |
            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, // usage
        VK_SHARING_MODE_EXCLUSIVE,               // sharingMode
        0,                                       // queueFamilyIndexCount
        nullptr,                                 // pQueueFamilyIndices
        VK_IMAGE_LAYOUT_UNDEFINED,               // initialLayout
    };

    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

    VK_CHECK(vmaCreateImage(
        ctx::allocator,
        &imageCreateInfo,
        &allocInfo,
        &this->multiSampleTargets_.color.image,
        &this->multiSampleTargets_.color.allocation,
        nullptr));

    VkImageViewCreateInfo imageViewCreateInfo = {
        VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO, // sType
        nullptr,                                  // pNext
        0,                                        // flags
        this->multiSampleTargets_.color.image,    // image
        VK_IMAGE_VIEW_TYPE_2D,                    // viewType
        this->swapchainImageFormat_,              // format
        {
            VK_COMPONENT_SWIZZLE_R, // r
            VK_COMPONENT_SWIZZLE_G, // g
            VK_COMPONENT_SWIZZLE_B, // b
            VK_COMPONENT_SWIZZLE_A, // a
        },                          // components
        {
            VK_IMAGE_ASPECT_COLOR_BIT, // aspectMask
            {},                        // baseMipLevel
            1,                         // levelCount
            {},                        // baseArrayLayer
            1,                         // layerCount
        },                             // subresourceRange
    };

    VK_CHECK(vkCreateImageView(
        ctx::device,
        &imageViewCreateInfo,
        nullptr,
        &this->multiSampleTargets_.color.view));
  }

  // Depth Target
  {
    VkImageCreateInfo imageCreateInfo = {
        VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO, // sType
        nullptr,                             // pNext
        0,                                   // flags
        VK_IMAGE_TYPE_2D,                    // imageType
        this->depthImageFormat_,             // format
        {
            this->swapchainExtent_.width,  // width
            this->swapchainExtent_.height, // height
            1,                             // depth
        },                                 // extent
        1,                                 // mipLevels
        1,                                 // arrayLayers
        this->msaaSamples_,                // samples
        VK_IMAGE_TILING_OPTIMAL,           // tiling
        VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT |
            VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, // usage
        VK_SHARING_MODE_EXCLUSIVE,                       // sharingMode
        0,                         // queueFamilyIndexCount
        nullptr,                   // pQueueFamilyIndices
        VK_IMAGE_LAYOUT_UNDEFINED, // initialLayout
    };

    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

    VK_CHECK(vmaCreateImage(
        ctx::allocator,
        &imageCreateInfo,
        &allocInfo,
        &this->multiSampleTargets_.depth.image,
        &this->multiSampleTargets_.depth.allocation,
        nullptr));

    VkImageViewCreateInfo imageViewCreateInfo = {
        VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO, // sType
        nullptr,                                  // pNext
        0,                                        // flags
        this->multiSampleTargets_.depth.image,    // image
        VK_IMAGE_VIEW_TYPE_2D,                    // viewType
        this->depthImageFormat_,                  // format
        {
            VK_COMPONENT_SWIZZLE_R, // r
            VK_COMPONENT_SWIZZLE_G, // g
            VK_COMPONENT_SWIZZLE_B, // b
            VK_COMPONENT_SWIZZLE_A, // a
        },                          // components
        {
            VK_IMAGE_ASPECT_DEPTH_BIT |
                VK_IMAGE_ASPECT_STENCIL_BIT, // aspectMask
            {},                              // baseMipLevel
            1,                               // levelCount
            {},                              // baseArrayLayer
            1,                               // layerCount
        },                                   // subresourceRange
    };

    VK_CHECK(vkCreateImageView(
        ctx::device,
        &imageViewCreateInfo,
        nullptr,
        &this->multiSampleTargets_.depth.view));
  }
}

void Window::createRenderPass() {
  VkAttachmentDescription attachmentDescriptions[4] = {
      // Multisampled color attachment
      VkAttachmentDescription{
          0,                                        // flags
          this->swapchainImageFormat_,              // format
          this->msaaSamples_,                       // samples
          VK_ATTACHMENT_LOAD_OP_CLEAR,              // loadOp
          VK_ATTACHMENT_STORE_OP_STORE,             // storeOp
          VK_ATTACHMENT_LOAD_OP_DONT_CARE,          // stencilLoadOp
          VK_ATTACHMENT_STORE_OP_DONT_CARE,         // stencilStoreOp
          VK_IMAGE_LAYOUT_UNDEFINED,                // initialLayout
          VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, // finalLayout
      },

      // Resolved color attachment
      VkAttachmentDescription{
          0,                                // flags
          this->swapchainImageFormat_,      // format
          VK_SAMPLE_COUNT_1_BIT,            // samples
          VK_ATTACHMENT_LOAD_OP_DONT_CARE,  // loadOp
          VK_ATTACHMENT_STORE_OP_STORE,     // storeOp
          VK_ATTACHMENT_LOAD_OP_DONT_CARE,  // stencilLoadOp
          VK_ATTACHMENT_STORE_OP_DONT_CARE, // stencilStoreOp
          VK_IMAGE_LAYOUT_UNDEFINED,        // initialLayout
          VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,  // finalLayout
      },

      // Multisampled depth attachment
      VkAttachmentDescription{
          0,                                                // flags
          this->depthImageFormat_,                          // format
          this->msaaSamples_,                               // samples
          VK_ATTACHMENT_LOAD_OP_CLEAR,                      // loadOp
          VK_ATTACHMENT_STORE_OP_DONT_CARE,                 // storeOp
          VK_ATTACHMENT_LOAD_OP_DONT_CARE,                  // stencilLoadOp
          VK_ATTACHMENT_STORE_OP_DONT_CARE,                 // stencilStoreOp
          VK_IMAGE_LAYOUT_UNDEFINED,                        // initialLayout
          VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, // finalLayout
      },

      // Resolved depth attachment
      VkAttachmentDescription{
          0,                                                // flags
          this->depthImageFormat_,                          // format
          VK_SAMPLE_COUNT_1_BIT,                            // samples
          VK_ATTACHMENT_LOAD_OP_DONT_CARE,                  // loadOp
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
      2,                                                // attachment
      VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, // layout
  };

  VkAttachmentReference resolveAttachmentReference = {
      1,                                        // attachment
      VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, // layout
  };

  VkSubpassDescription subpassDescription = {
      {},                              // flags
      VK_PIPELINE_BIND_POINT_GRAPHICS, // pipelineBindPoint
      0,                               // inputAttachmentCount
      nullptr,                         // pInputAttachments
      1,                               // colorAttachmentCount
      &colorAttachmentReference,       // pColorAttachments
      &resolveAttachmentReference,     // pResolveAttachments
      &depthAttachmentReference,       // pDepthStencilAttachment
      0,                               // preserveAttachmentCount
      nullptr,                         // pPreserveAttachments
  };

  if (this->msaaSamples_ == VK_SAMPLE_COUNT_1_BIT) {
    // Disable multisampled color clearing
    attachmentDescriptions[0].loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachmentDescriptions[0].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

    // Enable resolve color clearing
    attachmentDescriptions[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachmentDescriptions[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;

    // Disable multisampled depth clearing
    attachmentDescriptions[2].loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachmentDescriptions[2].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

    // Enable resolve depth clearing
    attachmentDescriptions[3].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachmentDescriptions[3].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

    colorAttachmentReference.attachment = 1;
    depthAttachmentReference.attachment = 3;
    subpassDescription.pResolveAttachments = nullptr;
  }

  VkSubpassDependency dependencies[2] = {
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
      ctx::device, &renderPassCreateInfo, nullptr, &this->renderPass_));
}

void Window::createImguiRenderPass() {
  VkAttachmentDescription attachment = {};
  attachment.format = this->swapchainImageFormat_;
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
      ctx::device, &renderPassCreateInfo, nullptr, &this->imguiRenderPass_));
}

void Window::initImgui() {
  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
  (void)io;

  ImGui_ImplSDL2_InitForVulkan(this->window_);

  // Setup Vulkan binding
  ImGui_ImplVulkan_InitInfo init_info = {};
  init_info.Instance = ctx::instance;
  init_info.PhysicalDevice = ctx::physicalDevice;
  init_info.Device = ctx::device;
  init_info.QueueFamily = ctx::graphicsQueueFamilyIndex;
  init_info.Queue = ctx::graphicsQueue;
  init_info.PipelineCache = VK_NULL_HANDLE;
  init_info.DescriptorPool = *ctx::descriptorManager.getPool(DESC_IMGUI);
  init_info.Allocator = nullptr;
  init_info.CheckVkResultFn = [](VkResult result) {
    if (result != VK_SUCCESS) {
      throw std::runtime_error("Failed to initialize IMGUI!");
    }
  };
  ImGui_ImplVulkan_Init(&init_info, this->imguiRenderPass_);

  // Setup style
  ImGui::StyleColorsDark();

  // Upload Fonts
  {
    // Use any command queue
    VkCommandPool commandPool = ctx::graphicsCommandPool;
    VkCommandBuffer commandBuffer =
        this->frameResources_[this->currentFrame_].commandBuffer;

    VK_CHECK(vkResetCommandPool(ctx::device, commandPool, 0));
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

    VK_CHECK(vkQueueSubmit(ctx::graphicsQueue, 1, &endInfo, VK_NULL_HANDLE));

    VK_CHECK(vkDeviceWaitIdle(ctx::device));

    ImGui_ImplVulkan_InvalidateFontUploadObjects();
  }
}

void Window::regenFramebuffer(
    VkFramebuffer &framebuffer, VkImageView &swapchainImageView) {
  vkDestroyFramebuffer(ctx::device, framebuffer, nullptr);

  VkImageView attachments[4]{
      this->multiSampleTargets_.color.view,
      swapchainImageView,
      this->multiSampleTargets_.depth.view,
      this->depthStencil_.view,
  };

  VkFramebufferCreateInfo createInfo = {
      VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,     // sType
      nullptr,                                       // pNext
      0,                                             // flags
      this->renderPass_,                             // renderPass
      static_cast<uint32_t>(ARRAYSIZE(attachments)), // attachmentCount
      attachments,                                   // pAttachments
      this->swapchainExtent_.width,                  // width
      this->swapchainExtent_.height,                 // height
      1,                                             // layers
  };

  VK_CHECK(
      vkCreateFramebuffer(ctx::device, &createInfo, nullptr, &framebuffer));
}

void Window::regenImguiFramebuffer(
    VkFramebuffer &framebuffer, VkImageView &swapchainImageView) {
  vkDestroyFramebuffer(ctx::device, framebuffer, nullptr);

  VkFramebufferCreateInfo createInfo = {
      VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO, // sType
      nullptr,                                   // pNext
      0,                                         // flags
      this->imguiRenderPass_,                    // renderPass
      1,                                         // attachmentCount
      &swapchainImageView,                       // pAttachments
      this->swapchainExtent_.width,              // width
      this->swapchainExtent_.height,             // height
      1,                                         // layers
  };

  VK_CHECK(
      vkCreateFramebuffer(ctx::device, &createInfo, nullptr, &framebuffer));
}

void Window::destroyResizables() {
  VK_CHECK(vkDeviceWaitIdle(ctx::device));

  for (auto &resources : this->frameResources_) {
    vkFreeCommandBuffers(
        ctx::device, ctx::graphicsCommandPool, 1, &resources.commandBuffer);
  }

  if (this->depthStencil_.image) {
    vkDestroyImageView(ctx::device, this->depthStencil_.view, nullptr);
    vmaDestroyImage(
        ctx::allocator,
        this->depthStencil_.image,
        this->depthStencil_.allocation);
    this->depthStencil_.image = nullptr;
    this->depthStencil_.allocation = VK_NULL_HANDLE;
  }

  if (this->multiSampleTargets_.color.image) {
    vkDestroyImageView(
        ctx::device, this->multiSampleTargets_.color.view, nullptr);
    vmaDestroyImage(
        ctx::allocator,
        this->multiSampleTargets_.color.image,
        this->multiSampleTargets_.color.allocation);
    this->multiSampleTargets_.color.image = nullptr;
    this->multiSampleTargets_.color.allocation = VK_NULL_HANDLE;
  }

  if (this->multiSampleTargets_.depth.image) {
    vkDestroyImageView(
        ctx::device, this->multiSampleTargets_.depth.view, nullptr);
    vmaDestroyImage(
        ctx::allocator,
        this->multiSampleTargets_.depth.image,
        this->multiSampleTargets_.depth.allocation);
    this->multiSampleTargets_.depth.image = nullptr;
    this->multiSampleTargets_.depth.allocation = VK_NULL_HANDLE;
  }

  vkDestroyRenderPass(ctx::device, this->imguiRenderPass_, nullptr);
  vkDestroyRenderPass(ctx::device, this->renderPass_, nullptr);
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
      fstl::log::debug("Using Immediate present mode");
      return presentMode;
    }
  }

  for (const auto &presentMode : presentModes) {
    if (presentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
      fstl::log::debug("Using mailbox present mode");
      return presentMode;
    }
  }

  for (const auto &presentMode : presentModes) {
    if (presentMode == VK_PRESENT_MODE_FIFO_KHR) {
      fstl::log::debug("Using FIFO present mode");
      return presentMode;
    }
  }

  fstl::log::fatal("FIFO present mode is not supported by the swapchain!");

  return static_cast<VkPresentModeKHR>(-1);
}
