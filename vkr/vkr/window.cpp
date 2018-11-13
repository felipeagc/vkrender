#include "window.hpp"
#include "commandbuffer.hpp"
#include "context.hpp"
#include <SDL2/SDL_vulkan.h>
#include <fstl/logging.hpp>
#include <imgui/imgui.h>
#include <imgui/imgui_impl_sdl.h>
#include <imgui/imgui_impl_vulkan.h>

using namespace vkr;

std::vector<const char *> Window::requiredVulkanExtensions;

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

  this->initVulkanExtensions();

  this->createVulkanSurface();

  // Lazily create vulkan context stuff
  Context::get().lazyInit(this->surface_);

  if (!Context::get().physicalDevice_.getSurfaceSupportKHR(
          Context::get().presentQueueFamilyIndex_, this->surface_)) {
    throw std::runtime_error(
        "Selected present queue does not support this window's surface");
  }

  this->maxMsaaSamples_ = Context::get().getMaxUsableSampleCount();

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

Window::~Window() {
  Context::getDevice().waitIdle();

  ImGui_ImplVulkan_Shutdown();
  ImGui_ImplSDL2_Shutdown();
  ImGui::DestroyContext();

  this->destroyResizables();

  for (auto &swapchainImageView : this->swapchainImageViews_) {
    Context::getDevice().destroy(swapchainImageView);
  }

  Context::getDevice().destroy(this->swapchain_);

  for (auto &frameResource : this->frameResources_) {
    Context::getDevice().destroy(frameResource.framebuffer);
    Context::getDevice().destroy(frameResource.imguiFramebuffer);
    Context::getDevice().destroy(frameResource.imageAvailableSemaphore);
    Context::getDevice().destroy(frameResource.renderingFinishedSemaphore);
    Context::getDevice().destroy(frameResource.fence);
  }

  Context::get().instance_.destroy(this->surface_);

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
  Context::getDevice().waitForFences(
      this->frameResources_[this->currentFrame_].fence, VK_TRUE, UINT64_MAX);

  Context::getDevice().resetFences(
      this->frameResources_[this->currentFrame_].fence);

  try {
    Context::getDevice().acquireNextImageKHR(
        this->swapchain_,
        UINT64_MAX,
        this->frameResources_[this->currentFrame_].imageAvailableSemaphore,
        {},
        &this->currentImageIndex_);
  } catch (const vk::OutOfDateKHRError &e) {
    this->updateSize();
  }

  vk::ImageSubresourceRange imageSubresourceRange{
      vk::ImageAspectFlagBits::eColor, // aspectMask
      0,                               // baseMipLevel
      1,                               // levelCount
      0,                               // baseArrayLayer
      1,                               // layerCount
  };

  this->regenFramebuffer(
      this->frameResources_[this->currentFrame_].framebuffer,
      this->swapchainImageViews_[this->currentImageIndex_]);

  this->regenImguiFramebuffer(
      this->frameResources_[this->currentFrame_].imguiFramebuffer,
      this->swapchainImageViews_[this->currentImageIndex_]);

  vk::CommandBufferBeginInfo beginInfo{
      vk::CommandBufferUsageFlagBits::eSimultaneousUse, // flags
      nullptr,                                          // pInheritanceInfo
  };

  auto &commandBuffer = this->frameResources_[this->currentFrame_].commandBuffer;

  commandBuffer.begin(beginInfo);

  if (Context::get().presentQueue_ != Context::get().graphicsQueue_) {
    vk::ImageMemoryBarrier barrierFromPresentToDraw = {
        vk::AccessFlagBits::eMemoryRead,                // srcAccessMask
        vk::AccessFlagBits::eMemoryRead,                // dstAccessMask
        vk::ImageLayout::eUndefined,                    // oldLayout
        vk::ImageLayout::eColorAttachmentOptimal,       // newLayout
        Context::get().presentQueueFamilyIndex_,         // srcQueueFamilyIndex
        Context::get().graphicsQueueFamilyIndex_,        // dstQueueFamilyIndex
        this->swapchainImages_[this->currentImageIndex_], // image
        imageSubresourceRange,                          // subresourceRange
    };

    commandBuffer.pipelineBarrier(
        vk::PipelineStageFlagBits::eColorAttachmentOutput,
        vk::PipelineStageFlagBits::eColorAttachmentOutput,
        {},
        {},
        {},
        barrierFromPresentToDraw);
  }

  std::array<float, 4> clearColor{
      this->clearColor.x,
      this->clearColor.y,
      this->clearColor.z,
      this->clearColor.w,
  };

  std::array<vk::ClearValue, 4> clearValues{
      vk::ClearValue{},
      vk::ClearValue{clearColor},
      vk::ClearValue{},
      vk::ClearValue{vk::ClearDepthStencilValue{1.0f, 0}},
  };

  if (this->msaaSamples_ != SampleCount::e1) {
    clearValues = {
        vk::ClearValue{clearColor},
        vk::ClearValue{clearColor},
        vk::ClearValue{vk::ClearDepthStencilValue{1.0f, 0}},
    };
  }

  vk::RenderPassBeginInfo renderPassBeginInfo{
      this->renderPass_,                                     // renderPass
      this->frameResources_[this->currentFrame_].framebuffer, // framebuffer
      {{0, 0}, this->swapchainExtent_},                      // renderArea
      static_cast<uint32_t>(clearValues.size()),            // clearValueCount
      clearValues.data(),                                   // pClearValues
  };

  commandBuffer.beginRenderPass(
      renderPassBeginInfo, vk::SubpassContents::eInline);

  vk::Viewport viewport{
      0.0f,                                             // x
      0.0f,                                             // y
      static_cast<float>(this->swapchainExtent_.width),  // width
      static_cast<float>(this->swapchainExtent_.height), // height
      0.0f,                                             // minDepth
      1.0f,                                             // maxDepth
  };

  vk::Rect2D scissor{{0, 0}, this->swapchainExtent_};
  commandBuffer.setViewport(0, viewport);

  commandBuffer.setScissor(0, scissor);

  this->imguiBeginFrame();

  // Draw
  drawFunction();

  this->imguiEndFrame();

  // End
  commandBuffer.endRenderPass();

  {
    vk::RenderPassBeginInfo imguiRenderPassBeginInfo{
        this->imguiRenderPass_, // renderPass
        this->frameResources_[this->currentFrame_]
            .imguiFramebuffer,           // framebuffer
        {{0, 0}, this->swapchainExtent_}, // renderArea
        0,                               // clearValueCount
        nullptr,                         // pClearValues
    };

    commandBuffer.beginRenderPass(
        imguiRenderPassBeginInfo, vk::SubpassContents::eInline);

    vk::Viewport viewport{
        0.0f,                                             // x
        0.0f,                                             // y
        static_cast<float>(this->swapchainExtent_.width),  // width
        static_cast<float>(this->swapchainExtent_.height), // height
        0.0f,                                             // minDepth
        1.0f,                                             // maxDepth
    };

    vk::Rect2D scissor{{0, 0}, this->swapchainExtent_};
    commandBuffer.setViewport(0, viewport);

    commandBuffer.setScissor(0, scissor);

    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);

    commandBuffer.endRenderPass();
  }

  if (Context::get().presentQueue_ != Context::get().graphicsQueue_) {
    vk::ImageMemoryBarrier barrierFromDrawToPresent{
        vk::AccessFlagBits::eMemoryRead,                // srcAccessMask
        vk::AccessFlagBits::eMemoryRead,                // dstAccessMask
        vk::ImageLayout::eColorAttachmentOptimal,       // oldLayout
        vk::ImageLayout::ePresentSrcKHR,                // newLayout
        Context::get().graphicsQueueFamilyIndex_,        // srcQueueFamilyIndex
        Context::get().presentQueueFamilyIndex_,         // dstQueueFamilyIndex
        this->swapchainImages_[this->currentImageIndex_], // image
        imageSubresourceRange,                          // subresourceRange
    };

    commandBuffer.pipelineBarrier(
        vk::PipelineStageFlagBits::eColorAttachmentOutput, // srcStageMask
        vk::PipelineStageFlagBits::eBottomOfPipe,          // dstStageMask
        {},                                                // dependencyFlags
        nullptr,                                           // memoryBarriers
        nullptr,                 // bufferMemoryBarriers
        barrierFromDrawToPresent // imageMemoryBarriers
    );
  }

  commandBuffer.end();

  // Present
  vk::PipelineStageFlags waitDstStageMask =
      vk::PipelineStageFlagBits::eColorAttachmentOutput;

  vk::SubmitInfo submitInfo = {
      1, // waitSemaphoreCount
      &this->frameResources_[this->currentFrame_]
           .imageAvailableSemaphore, // pWaitSemaphores
      &waitDstStageMask,             // pWaitDstStageMask
      1,                             // commandBufferCount
      &commandBuffer,                // pCommandBuffers
      1,                             // signalSemaphoreCount
      &this->frameResources_[this->currentFrame_]
           .renderingFinishedSemaphore, // pSignalSemaphores
  };

  Context::get().graphicsQueue_.submit(
      submitInfo, this->frameResources_[this->currentFrame_].fence);

  vk::PresentInfoKHR presentInfo{
      1, // waitSemaphoreCount
      &this->frameResources_[this->currentFrame_]
           .renderingFinishedSemaphore, // pWaitSemaphores
      1,                                // swapchainCount
      &this->swapchain_,                 // pSwapchains
      &this->currentImageIndex_,         // pImageIndices
      nullptr,                          // pResults
  };

  try {
    Context::get().presentQueue_.presentKHR(presentInfo);
  } catch (const vk::OutOfDateKHRError &e) {
    this->updateSize();
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

SampleCount Window::getMaxMSAASamples() const { return this->maxMsaaSamples_; }

SampleCount Window::getMSAASamples() const { return this->msaaSamples_; }

void Window::setMSAASamples(SampleCount sampleCount) {
  if (sampleCount <= this->maxMsaaSamples_) {
    this->msaaSamples_ = sampleCount;

    // Recreate stuff using new sample count
    this->updateSize();
  } else {
    throw std::runtime_error("Invalid MSAA sample count");
  }
}

int Window::getCurrentFrameIndex() const { return this->currentFrame_; }

CommandBuffer Window::getCurrentCommandBuffer() {
  return CommandBuffer{this->frameResources_[this->currentFrame_].commandBuffer};
}

void Window::imguiBeginFrame() {
  ImGui_ImplVulkan_NewFrame();
  ImGui_ImplSDL2_NewFrame(window_);
  ImGui::NewFrame();
}

void Window::imguiEndFrame() { ImGui::Render(); }

void Window::initVulkanExtensions() const {
  uint32_t sdlExtensionCount = 0;
  SDL_Vulkan_GetInstanceExtensions(this->window_, &sdlExtensionCount, nullptr);
  std::vector<const char *> sdlExtensions(sdlExtensionCount);
  SDL_Vulkan_GetInstanceExtensions(
      this->window_, &sdlExtensionCount, sdlExtensions.data());
  Window::requiredVulkanExtensions = sdlExtensions;
}

void Window::createVulkanSurface() {
  if (!SDL_Vulkan_CreateSurface(
          this->window_,
          static_cast<VkInstance>(Context::get().instance_),
          reinterpret_cast<VkSurfaceKHR *>(&this->surface_))) {
    throw std::runtime_error(
        "Failed to create window surface: " + std::string(SDL_GetError()));
  }
}

void Window::createSyncObjects() {
  for (auto &resources : this->frameResources_) {
    resources.imageAvailableSemaphore =
        Context::getDevice().createSemaphore({});
    resources.renderingFinishedSemaphore =
        Context::getDevice().createSemaphore({});
    resources.fence =
        Context::getDevice().createFence({vk::FenceCreateFlagBits::eSignaled});
  }
}

void Window::createSwapchain(uint32_t width, uint32_t height) {
  for (const auto &imageView : this->swapchainImageViews_) {
    if (imageView) {
      Context::getDevice().destroy(imageView);
    }
  }
  this->swapchainImageViews_.clear();

  auto surfaceCapabilities =
      Context::get().physicalDevice_.getSurfaceCapabilitiesKHR(this->surface_);
  auto surfaceFormats =
      Context::get().physicalDevice_.getSurfaceFormatsKHR(this->surface_);
  auto presentModes =
      Context::get().physicalDevice_.getSurfacePresentModesKHR(this->surface_);

  auto desiredNumImages = getSwapchainNumImages(surfaceCapabilities);
  auto desiredFormat = getSwapchainFormat(surfaceFormats);
  auto desiredExtent = getSwapchainExtent(width, height, surfaceCapabilities);
  auto desiredUsage = getSwapchainUsageFlags(surfaceCapabilities);
  auto desiredTransform = getSwapchainTransform(surfaceCapabilities);
  auto desiredPresentMode = getSwapchainPresentMode(presentModes);

  vk::SwapchainKHR oldSwapchain = this->swapchain_;

  vk::SwapchainCreateInfoKHR createInfo{
      {}, // flags
      this->surface_,
      desiredNumImages,                       // minImageCount
      desiredFormat.format,                   // imageFormat
      desiredFormat.colorSpace,               // imageColorSpace
      desiredExtent,                          // imageExtent
      1,                                      // imageArrayLayers
      desiredUsage,                           // imageUsage
      vk::SharingMode::eExclusive,            // imageSharingMode
      0,                                      // queueFamilyIndexCount
      nullptr,                                // pQueueFamiylIndices
      desiredTransform,                       // preTransform
      vk::CompositeAlphaFlagBitsKHR::eOpaque, // compositeAlpha
      desiredPresentMode,                     // presentMode
      VK_TRUE,                                // clipped
      oldSwapchain                            // oldSwapchain
  };

  this->swapchain_ = Context::getDevice().createSwapchainKHR(createInfo);

  if (oldSwapchain) {
    Context::getDevice().destroy(oldSwapchain);
  }

  this->swapchainImageFormat_ = desiredFormat.format;
  this->swapchainExtent_ = desiredExtent;

  this->swapchainImages_ =
      Context::getDevice().getSwapchainImagesKHR(this->swapchain_);
}

void Window::createSwapchainImageViews() {
  this->swapchainImageViews_.resize(this->swapchainImages_.size());

  for (size_t i = 0; i < swapchainImages_.size(); i++) {
    vk::ImageViewCreateInfo createInfo{
        {},
        this->swapchainImages_[i],
        vk::ImageViewType::e2D,
        this->swapchainImageFormat_,
        {
            vk::ComponentSwizzle::eIdentity, // r
            vk::ComponentSwizzle::eIdentity, // g
            vk::ComponentSwizzle::eIdentity, // b
            vk::ComponentSwizzle::eIdentity, // a
        },
        {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1}};

    this->swapchainImageViews_[i] =
        Context::getDevice().createImageView(createInfo);
  }
}

void Window::allocateGraphicsCommandBuffers() {
  auto commandBuffers = Context::getDevice().allocateCommandBuffers(
      {Context::get().graphicsCommandPool_,
       vk::CommandBufferLevel::ePrimary,
       MAX_FRAMES_IN_FLIGHT});

  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    this->frameResources_[i].commandBuffer = commandBuffers[i];
  }
}

void Window::createDepthStencilResources() {
  std::array<vk::Format, 5> depthFormats = {vk::Format::eD32SfloatS8Uint,
                                            vk::Format::eD32Sfloat,
                                            vk::Format::eD24UnormS8Uint,
                                            vk::Format::eD16UnormS8Uint,
                                            vk::Format::eD16Unorm};
  bool validDepthFormat = false;
  for (auto &format : depthFormats) {
    vk::FormatProperties formatProps =
        Context::getPhysicalDevice().getFormatProperties(format);
    if (formatProps.optimalTilingFeatures &
        vk::FormatFeatureFlagBits::eDepthStencilAttachment) {
      this->depthImageFormat_ = format;
      validDepthFormat = true;
      break;
    }
  }
  assert(validDepthFormat);

  vk::ImageCreateInfo imageCreateInfo{
      {},
      vk::ImageType::e2D,
      this->depthImageFormat_,
      {
          this->swapchainExtent_.width,
          this->swapchainExtent_.height,
          1,
      },
      1,
      1,
      vk::SampleCountFlagBits::e1,
      vk::ImageTiling::eOptimal,
      vk::ImageUsageFlagBits::eDepthStencilAttachment |
          vk::ImageUsageFlagBits::eSampled,
      vk::SharingMode::eExclusive,
      0,
      nullptr,
      vk::ImageLayout::eUndefined,
  };

  VmaAllocationCreateInfo allocInfo{};
  allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

  if (vmaCreateImage(
          Context::get().allocator_,
          reinterpret_cast<VkImageCreateInfo *>(&imageCreateInfo),
          &allocInfo,
          reinterpret_cast<VkImage *>(&this->depthStencil_.image),
          &this->depthStencil_.allocation,
          nullptr) != VK_SUCCESS) {
    throw std::runtime_error("Failed to create the depth stencil image");
  }

  vk::ImageViewCreateInfo imageViewCreateInfo{
      {},
      this->depthStencil_.image,
      vk::ImageViewType::e2D,
      this->depthImageFormat_,
      {
          vk::ComponentSwizzle::eIdentity, // r
          vk::ComponentSwizzle::eIdentity, // g
          vk::ComponentSwizzle::eIdentity, // b
          vk::ComponentSwizzle::eIdentity, // a
      },                                   // components
      {
          vk::ImageAspectFlagBits::eDepth |
              vk::ImageAspectFlagBits::eStencil, // aspectMask
          0,                                     // baseMipLevel
          1,                                     // levelCount
          0,                                     // baseArrayLayer
          1,                                     // layerCount
      },                                         // subresourceRange
  };

  this->depthStencil_.view =
      Context::getDevice().createImageView(imageViewCreateInfo);
}

void Window::createMultisampleTargets() {
  // Color target
  {
    vk::ImageCreateInfo imageCreateInfo{
        {},                         // flags
        vk::ImageType::e2D,         // imageType
        this->swapchainImageFormat_, // format
        {
            this->swapchainExtent_.width,  // width
            this->swapchainExtent_.height, // height
            1,                            // depth
        },                                // extent
        1,                                // mipLevels
        1,                                // arrayLayers
        this->msaaSamples_,                // samples
        vk::ImageTiling::eOptimal,        // tiling
        vk::ImageUsageFlagBits::eTransientAttachment |
            vk::ImageUsageFlagBits::eColorAttachment, // usage
        vk::SharingMode::eExclusive,                  // sharingMode
        0,                                            // queueFamilyIndexCount
        nullptr,                                      // pQueueFamilyIndices
        vk::ImageLayout::eUndefined,                  // initialLayout
    };

    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

    if (vmaCreateImage(
            Context::get().allocator_,
            reinterpret_cast<VkImageCreateInfo *>(&imageCreateInfo),
            &allocInfo,
            reinterpret_cast<VkImage *>(&this->multiSampleTargets_.color.image),
            &this->multiSampleTargets_.color.allocation,
            nullptr) != VK_SUCCESS) {
      throw std::runtime_error(
          "Failed to create the multisample target color image");
    }

    vk::ImageViewCreateInfo imageViewCreateInfo{
        {},                                   // flags
        this->multiSampleTargets_.color.image, // image
        vk::ImageViewType::e2D,               // viewType
        this->swapchainImageFormat_,           // format
        {
            vk::ComponentSwizzle::eR, // r
            vk::ComponentSwizzle::eG, // g
            vk::ComponentSwizzle::eB, // b
            vk::ComponentSwizzle::eA, // a
        },                            // components
        {
            vk::ImageAspectFlagBits::eColor, // aspectMask
            {},                              // baseMipLevel
            1,                               // levelCount
            {},                              // baseArrayLayer
            1,                               // layerCount
        },                                   // subresourceRange
    };

    this->multiSampleTargets_.color.view =
        Context::getDevice().createImageView(imageViewCreateInfo);
  }

  // Depth Target
  {
    vk::ImageCreateInfo imageCreateInfo{
        {},                     // flags
        vk::ImageType::e2D,     // imageType
        this->depthImageFormat_, // format
        {
            this->swapchainExtent_.width,  // width
            this->swapchainExtent_.height, // height
            1,                            // depth
        },                                // extent
        1,                                // mipLevels
        1,                                // arrayLayers
        this->msaaSamples_,                // samples
        vk::ImageTiling::eOptimal,        // tiling
        vk::ImageUsageFlagBits::eTransientAttachment |
            vk::ImageUsageFlagBits::eDepthStencilAttachment, // usage
        vk::SharingMode::eExclusive,                         // sharingMode
        0,                           // queueFamilyIndexCount
        nullptr,                     // pQueueFamilyIndices
        vk::ImageLayout::eUndefined, // initialLayout
    };

    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

    if (vmaCreateImage(
            Context::get().allocator_,
            reinterpret_cast<VkImageCreateInfo *>(&imageCreateInfo),
            &allocInfo,
            reinterpret_cast<VkImage *>(&this->multiSampleTargets_.depth.image),
            &this->multiSampleTargets_.depth.allocation,
            nullptr) != VK_SUCCESS) {
      throw std::runtime_error(
          "Failed to create the multisample target depth image");
    }

    vk::ImageViewCreateInfo imageViewCreateInfo{
        {},                                   // flags
        this->multiSampleTargets_.depth.image, // image
        vk::ImageViewType::e2D,               // viewType
        this->depthImageFormat_,               // format
        {
            vk::ComponentSwizzle::eR, // r
            vk::ComponentSwizzle::eG, // g
            vk::ComponentSwizzle::eB, // b
            vk::ComponentSwizzle::eA, // a
        },                            // components
        {
            vk::ImageAspectFlagBits::eDepth |
                vk::ImageAspectFlagBits::eStencil, // aspectMask
            {},                                    // baseMipLevel
            1,                                     // levelCount
            {},                                    // baseArrayLayer
            1,                                     // layerCount
        },                                         // subresourceRange
    };

    this->multiSampleTargets_.depth.view =
        Context::getDevice().createImageView(imageViewCreateInfo);
  }
}

void Window::createRenderPass() {
  std::array<vk::AttachmentDescription, 4> attachmentDescriptions{
      // Multisampled color attachment
      vk::AttachmentDescription{
          {},                                       // flags
          this->swapchainImageFormat_,               // format
          this->msaaSamples_,                        // samples
          vk::AttachmentLoadOp::eClear,             // loadOp
          vk::AttachmentStoreOp::eStore,            // storeOp
          vk::AttachmentLoadOp::eDontCare,          // stencilLoadOp
          vk::AttachmentStoreOp::eDontCare,         // stencilStoreOp
          vk::ImageLayout::eUndefined,              // initialLayout
          vk::ImageLayout::eColorAttachmentOptimal, // finalLayout
      },

      // Resolved color attachment
      vk::AttachmentDescription{
          {},                               // flags
          this->swapchainImageFormat_,       // format
          vk::SampleCountFlagBits::e1,      // samples
          vk::AttachmentLoadOp::eDontCare,  // loadOp
          vk::AttachmentStoreOp::eStore,    // storeOp
          vk::AttachmentLoadOp::eDontCare,  // stencilLoadOp
          vk::AttachmentStoreOp::eDontCare, // stencilStoreOp
          vk::ImageLayout::eUndefined,      // initialLayout
          vk::ImageLayout::ePresentSrcKHR,  // finalLayout
      },

      // Multisampled depth attachment
      vk::AttachmentDescription{
          {},                                              // flags
          this->depthImageFormat_,                          // format
          this->msaaSamples_,                               // samples
          vk::AttachmentLoadOp::eClear,                    // loadOp
          vk::AttachmentStoreOp::eDontCare,                // storeOp
          vk::AttachmentLoadOp::eDontCare,                 // stencilLoadOp
          vk::AttachmentStoreOp::eDontCare,                // stencilStoreOp
          vk::ImageLayout::eUndefined,                     // initialLayout
          vk::ImageLayout::eDepthStencilAttachmentOptimal, // finalLayout
      },

      // Resolved depth attachment
      vk::AttachmentDescription{
          {},                                              // flags
          this->depthImageFormat_,                          // format
          vk::SampleCountFlagBits::e1,                     // samples
          vk::AttachmentLoadOp::eDontCare,                 // loadOp
          vk::AttachmentStoreOp::eStore,                   // storeOp
          vk::AttachmentLoadOp::eDontCare,                 // stencilLoadOp
          vk::AttachmentStoreOp::eDontCare,                // stencilStoreOp
          vk::ImageLayout::eUndefined,                     // initialLayout
          vk::ImageLayout::eDepthStencilAttachmentOptimal, // finalLayout
      },
  };

  std::array<vk::AttachmentReference, 1> colorAttachmentReferences{
      vk::AttachmentReference{
          0,                                        // attachment
          vk::ImageLayout::eColorAttachmentOptimal, // layout
      },
  };

  std::array<vk::AttachmentReference, 1> depthAttachmentReferences{
      vk::AttachmentReference{
          2,                                               // attachment
          vk::ImageLayout::eDepthStencilAttachmentOptimal, // layout
      },
  };

  std::array<vk::AttachmentReference, 1> resolveAttachmentReferences{
      vk::AttachmentReference{
          1,                                        // attachment
          vk::ImageLayout::eColorAttachmentOptimal, // layout
      },
  };

  std::array<vk::SubpassDescription, 1> subpassDescriptions{
      vk::SubpassDescription{
          {},                                 // flags
          vk::PipelineBindPoint::eGraphics,   // pipelineBindPoint
          0,                                  // inputAttachmentCount
          nullptr,                            // pInputAttachments
          1,                                  // colorAttachmentCount
          colorAttachmentReferences.data(),   // pColorAttachments
          resolveAttachmentReferences.data(), // pResolveAttachments
          depthAttachmentReferences.data(),   // pDepthStencilAttachment
          0,                                  // preserveAttachmentCount
          nullptr,                            // pPreserveAttachments
      },
  };

  if (this->msaaSamples_ == SampleCount::e1) {
    // Disable multisampled color clearing
    attachmentDescriptions[0].loadOp = vk::AttachmentLoadOp::eDontCare;
    attachmentDescriptions[0].storeOp = vk::AttachmentStoreOp::eDontCare;

    // Enable resolve color clearing
    attachmentDescriptions[1].loadOp = vk::AttachmentLoadOp::eClear;
    attachmentDescriptions[1].storeOp = vk::AttachmentStoreOp::eStore;

    // Disable multisampled depth clearing
    attachmentDescriptions[2].loadOp = vk::AttachmentLoadOp::eDontCare;
    attachmentDescriptions[2].storeOp = vk::AttachmentStoreOp::eDontCare;

    // Enable resolve depth clearing
    attachmentDescriptions[3].loadOp = vk::AttachmentLoadOp::eClear;
    attachmentDescriptions[3].storeOp = vk::AttachmentStoreOp::eDontCare;

    colorAttachmentReferences[0].attachment = 1;
    depthAttachmentReferences[0].attachment = 3;
    subpassDescriptions[0].pResolveAttachments = nullptr;
  }

  std::array<vk::SubpassDependency, 2> dependencies{
      vk::SubpassDependency{
          VK_SUBPASS_EXTERNAL,                               // srcSubpass
          0,                                                 // dstSubpass
          vk::PipelineStageFlagBits::eBottomOfPipe,          // srcStageMask
          vk::PipelineStageFlagBits::eColorAttachmentOutput, // dstStageMask
          vk::AccessFlagBits::eMemoryRead,                   // srcAccessMask
          vk::AccessFlagBits::eColorAttachmentRead |
              vk::AccessFlagBits::eColorAttachmentWrite, // dstAccessMask
          vk::DependencyFlagBits::eByRegion,             // dependencyFlags
      },
      vk::SubpassDependency{
          0,                                                 // srcSubpass
          VK_SUBPASS_EXTERNAL,                               // dstSubpass
          vk::PipelineStageFlagBits::eColorAttachmentOutput, // srcStageMask
          vk::PipelineStageFlagBits::eBottomOfPipe,          // dstStageMask
          vk::AccessFlagBits::eColorAttachmentRead |
              vk::AccessFlagBits::eColorAttachmentWrite, // srcAccessMask
          vk::AccessFlagBits::eMemoryRead,               // dstAccessMask
          vk::DependencyFlagBits::eByRegion,             // dependencyFlags
      },
  };

  vk::RenderPassCreateInfo renderPassCreateInfo{
      {},                                                   // flags
      static_cast<uint32_t>(attachmentDescriptions.size()), // attachmentCount
      attachmentDescriptions.data(),                        // pAttachments
      1,                                                    // subpassCount
      subpassDescriptions.data(),                           // pSubpasses
      static_cast<uint32_t>(dependencies.size()),           // dependencyCount
      dependencies.data(),                                  // pDependencies
  };

  this->renderPass_ =
      Context::getDevice().createRenderPass(renderPassCreateInfo);
}

void Window::createImguiRenderPass() {
  vk::AttachmentDescription attachment{};
  attachment.format = this->swapchainImageFormat_;
  attachment.samples = vk::SampleCountFlagBits::e1;
  attachment.loadOp = vk::AttachmentLoadOp::eDontCare;
  attachment.storeOp = vk::AttachmentStoreOp::eStore;
  attachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
  attachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
  attachment.initialLayout = vk::ImageLayout::eUndefined;
  attachment.finalLayout = vk::ImageLayout::ePresentSrcKHR;

  vk::AttachmentReference color_attachment{};
  color_attachment.attachment = 0;
  color_attachment.layout = vk::ImageLayout::eColorAttachmentOptimal;

  vk::SubpassDescription subpass{};
  subpass.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
  subpass.colorAttachmentCount = 1;
  subpass.pColorAttachments = &color_attachment;

  vk::SubpassDependency dependency{};
  dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
  dependency.dstSubpass = 0;
  dependency.srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
  dependency.dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
  dependency.srcAccessMask = {};
  dependency.dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;

  vk::RenderPassCreateInfo renderPassCreateInfo{
      {},          // flags
      1,           // attachmentCount
      &attachment, // pAttachments
      1,           // subpassCount
      &subpass,    // pSubpasses
      1,           // dependencyCount
      &dependency, // pDependencies
  };

  this->imguiRenderPass_ =
      Context::getDevice().createRenderPass(renderPassCreateInfo);
}

void Window::initImgui() {
  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
  (void)io;

  ImGui_ImplSDL2_InitForVulkan(this->window_);

  // Setup Vulkan binding
  ImGui_ImplVulkan_InitInfo init_info = {};
  init_info.Instance = Context::get().instance_;
  init_info.PhysicalDevice = Context::getPhysicalDevice();
  init_info.Device = Context::getDevice();
  init_info.QueueFamily = Context::get().graphicsQueueFamilyIndex_;
  init_info.Queue = Context::get().graphicsQueue_;
  init_info.PipelineCache = VK_NULL_HANDLE;
  init_info.DescriptorPool =
      *Context::getDescriptorManager().getPool(DESC_IMGUI);
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
    vk::CommandPool commandPool = Context::get().graphicsCommandPool_;
    vk::CommandBuffer commandBuffer =
        this->frameResources_[this->currentFrame_].commandBuffer;

    Context::getDevice().resetCommandPool(commandPool, {});
    vk::CommandBufferBeginInfo beginInfo{
        vk::CommandBufferUsageFlagBits::eOneTimeSubmit, // flags
    };

    commandBuffer.begin(beginInfo);

    ImGui_ImplVulkan_CreateFontsTexture(commandBuffer);

    vk::SubmitInfo endInfo{};
    endInfo.commandBufferCount = 1;
    endInfo.pCommandBuffers = &commandBuffer;

    commandBuffer.end();

    Context::get().graphicsQueue_.submit(endInfo, {});

    Context::getDevice().waitIdle();

    ImGui_ImplVulkan_InvalidateFontUploadObjects();
  }
}

void Window::regenFramebuffer(
    vk::Framebuffer &framebuffer, vk::ImageView &swapchainImageView) {
  Context::getDevice().destroy(framebuffer);

  std::array<vk::ImageView, 4> attachments{
      this->multiSampleTargets_.color.view,
      swapchainImageView,
      this->multiSampleTargets_.depth.view,
      this->depthStencil_.view,
  };

  vk::FramebufferCreateInfo createInfo = {
      {},                                        // flags
      this->renderPass_,                          // renderPass
      static_cast<uint32_t>(attachments.size()), // attachmentCount
      attachments.data(),                        // pAttachments
      this->swapchainExtent_.width,               // width
      this->swapchainExtent_.height,              // height
      1,                                         // layers
  };

  framebuffer = Context::getDevice().createFramebuffer(createInfo);
}

void Window::regenImguiFramebuffer(
    vk::Framebuffer &framebuffer, vk::ImageView &swapchainImageView) {
  Context::getDevice().destroy(framebuffer);

  std::array<vk::ImageView, 1> attachments{
      swapchainImageView,
  };

  vk::FramebufferCreateInfo createInfo = {
      {},                                        // flags
      this->imguiRenderPass_,                     // renderPass
      static_cast<uint32_t>(attachments.size()), // attachmentCount
      attachments.data(),                        // pAttachments
      this->swapchainExtent_.width,               // width
      this->swapchainExtent_.height,              // height
      1,                                         // layers
  };

  framebuffer = Context::getDevice().createFramebuffer(createInfo);
}

void Window::destroyResizables() {
  Context::getDevice().waitIdle();

  for (auto &resources : this->frameResources_) {
    Context::getDevice().freeCommandBuffers(
        Context::get().graphicsCommandPool_, resources.commandBuffer);
  }

  if (this->depthStencil_.image) {
    Context::getDevice().destroy(this->depthStencil_.view);
    vmaDestroyImage(
        Context::get().allocator_,
        this->depthStencil_.image,
        this->depthStencil_.allocation);
    this->depthStencil_.image = nullptr;
    this->depthStencil_.allocation = VK_NULL_HANDLE;
  }

  if (this->multiSampleTargets_.color.image) {
    Context::getDevice().destroy(this->multiSampleTargets_.color.view);
    vmaDestroyImage(
        Context::get().allocator_,
        this->multiSampleTargets_.color.image,
        this->multiSampleTargets_.color.allocation);
    this->multiSampleTargets_.color.image = nullptr;
    this->multiSampleTargets_.color.allocation = VK_NULL_HANDLE;
  }

  if (this->multiSampleTargets_.depth.image) {
    Context::getDevice().destroy(this->multiSampleTargets_.depth.view);
    vmaDestroyImage(
        Context::get().allocator_,
        this->multiSampleTargets_.depth.image,
        this->multiSampleTargets_.depth.allocation);
    this->multiSampleTargets_.depth.image = nullptr;
    this->multiSampleTargets_.depth.allocation = VK_NULL_HANDLE;
  }

  Context::getDevice().destroy(this->imguiRenderPass_);
  Context::getDevice().destroy(this->renderPass_);
}

uint32_t Window::getSwapchainNumImages(
    const vk::SurfaceCapabilitiesKHR &surfaceCapabilities) {
  uint32_t imageCount = surfaceCapabilities.minImageCount + 1;

  if (surfaceCapabilities.maxImageCount > 0 &&
      imageCount > surfaceCapabilities.maxImageCount) {
    imageCount = surfaceCapabilities.maxImageCount;
  }

  return imageCount;
}

vk::SurfaceFormatKHR
Window::getSwapchainFormat(const std::vector<vk::SurfaceFormatKHR> &formats) {
  if (formats.size() == 1 && formats[0].format == vk::Format::eUndefined) {
    return {vk::Format::eR8G8B8A8Unorm, vk::ColorSpaceKHR::eSrgbNonlinear};
  }

  for (const auto &format : formats) {
    if (format.format == vk::Format::eR8G8B8A8Unorm) {
      return format;
    }
  }

  return formats[0];
}

vk::Extent2D Window::getSwapchainExtent(
    uint32_t width,
    uint32_t height,
    const vk::SurfaceCapabilitiesKHR &surfaceCapabilities) {
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

vk::ImageUsageFlags Window::getSwapchainUsageFlags(
    const vk::SurfaceCapabilitiesKHR &surfaceCapabilities) {
  if (surfaceCapabilities.supportedUsageFlags &
      vk::ImageUsageFlagBits::eTransferDst) {
    return vk::ImageUsageFlagBits::eColorAttachment |
           vk::ImageUsageFlagBits::eTransferDst;
  }

  fstl::log::fatal(
      "VK_IMAGE_USAGE_TRANSFER_DST image usage is not supported by the "
      "swapchain!\n"
      "Supported swapchain image usages include:\n"
      "{}\n{}\n{}\n{}\n{}\n{}\n{}\n{}\n",
      (surfaceCapabilities.supportedUsageFlags &
               vk::ImageUsageFlagBits::eTransferSrc
           ? "    vk::ImageUsageFlagBits::TRANSFER_SRC\n"
           : ""),
      (surfaceCapabilities.supportedUsageFlags &
               vk::ImageUsageFlagBits::eTransferDst
           ? "    vk::ImageUsageFlagBits::TRANSFER_DST\n"
           : ""),
      (surfaceCapabilities.supportedUsageFlags &
               vk::ImageUsageFlagBits::eSampled
           ? "    vk::ImageUsageFlagBits::SAMPLED\n"
           : ""),
      (surfaceCapabilities.supportedUsageFlags &
               vk::ImageUsageFlagBits::eStorage
           ? "    vk::ImageUsageFlagBits::STORAGE\n"
           : ""),
      (surfaceCapabilities.supportedUsageFlags &
               vk::ImageUsageFlagBits::eColorAttachment
           ? "    vk::ImageUsageFlagBits::COLOR_ATTACHMENT\n"
           : ""),
      (surfaceCapabilities.supportedUsageFlags &
               vk::ImageUsageFlagBits::eDepthStencilAttachment
           ? "    vk::ImageUsageFlagBits::DEPTH_STENCIL_ATTACHMENT\n"
           : ""),
      (surfaceCapabilities.supportedUsageFlags &
               vk::ImageUsageFlagBits::eTransientAttachment
           ? "    vk::ImageUsageFlagBits::TRANSIENT_ATTACHMENT\n"
           : ""),
      (surfaceCapabilities.supportedUsageFlags &
               vk::ImageUsageFlagBits::eInputAttachment
           ? "    vk::ImageUsageFlagBits::INPUT_ATTACHMENT"
           : ""));

  return static_cast<vk::ImageUsageFlags>(-1);
}

vk::SurfaceTransformFlagBitsKHR Window::getSwapchainTransform(
    const vk::SurfaceCapabilitiesKHR &surfaceCapabilities) {
  if (surfaceCapabilities.supportedTransforms &
      vk::SurfaceTransformFlagBitsKHR::eIdentity) {
    return vk::SurfaceTransformFlagBitsKHR::eIdentity;
  } else {
    return surfaceCapabilities.currentTransform;
  }
}

vk::PresentModeKHR Window::getSwapchainPresentMode(
    const std::vector<vk::PresentModeKHR> &presentModes) {
  for (const auto &presentMode : presentModes) {
    if (presentMode == vk::PresentModeKHR::eImmediate) {
      fstl::log::debug("Using Immediate present mode");
      return presentMode;
    }
  }

  for (const auto &presentMode : presentModes) {
    if (presentMode == vk::PresentModeKHR::eMailbox) {
      fstl::log::debug("Using mailbox present mode");
      return presentMode;
    }
  }

  for (const auto &presentMode : presentModes) {
    if (presentMode == vk::PresentModeKHR::eFifo) {
      fstl::log::debug("Using FIFO present mode");
      return presentMode;
    }
  }

  fstl::log::fatal("FIFO present mode is not supported by the swapchain!");

  return static_cast<vk::PresentModeKHR>(-1);
}
