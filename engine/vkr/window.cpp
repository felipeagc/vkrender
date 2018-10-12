#include "window.hpp"
#include "commandbuffer.hpp"
#include "context.hpp"
#include "logging.hpp"
#include <SDL2/SDL_vulkan.h>

using namespace vkr;

std::vector<const char *> Window::requiredVulkanExtensions;

Window::Window(const char *title, uint32_t width, uint32_t height) {
  auto subsystems = SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_TIMER;
  if (subsystems == SDL_WasInit(0)) {
    SDL_Init(subsystems);
  }

  this->window = SDL_CreateWindow(
      title,
      SDL_WINDOWPOS_CENTERED,
      SDL_WINDOWPOS_CENTERED,
      width,
      height,
      SDL_WINDOW_SHOWN | SDL_WINDOW_VULKAN);

  if (this->window == nullptr) {
    throw std::runtime_error("Failed to create SDL window");
  }

  this->initVulkanExtensions();

  this->createVulkanSurface();

  // Lazily create vulkan context stuff
  Context::get().lazyInit(this->surface);

  if (!Context::get().physicalDevice.getSurfaceSupportKHR(
          Context::get().presentQueueFamilyIndex, this->surface)) {
    throw std::runtime_error(
        "Selected present queue does not support this window's surface");
  }

  this->createSyncObjects();

  this->createSwapchain(this->getWidth(), this->getHeight());
  this->createSwapchainImageViews();

  this->allocateGraphicsCommandBuffers();

  this->createDepthResources();

  this->createRenderPass();
}

Window::~Window() {
  Context::getDevice().waitIdle();

  this->destroyResizables();

  for (auto &swapchainImageView : this->swapchainImageViews) {
    Context::getDevice().destroy(swapchainImageView);
  }

  Context::getDevice().destroy(this->swapchain);

  for (auto &frameResource : this->frameResources) {
    Context::getDevice().destroy(frameResource.framebuffer);
    Context::getDevice().destroy(frameResource.imageAvailableSemaphore);
    Context::getDevice().destroy(frameResource.renderingFinishedSemaphore);
    Context::getDevice().destroy(frameResource.fence);
  }

  Context::get().instance.destroy(this->surface);

  SDL_DestroyWindow(this->window);
}

SDL_Event Window::pollEvent() {
  SDL_Event event;
  SDL_PollEvent(&event);
  return event;
}

void Window::present(std::function<void(CommandBuffer &)> drawFunction) {
  this->lastTicks = SDL_GetTicks();

  // Begin
  Context::getDevice().waitForFences(
      this->frameResources[this->currentFrame].fence, VK_TRUE, UINT64_MAX);

  Context::getDevice().resetFences(
      this->frameResources[this->currentFrame].fence);

  try {
    Context::getDevice().acquireNextImageKHR(
        this->swapchain,
        UINT64_MAX,
        this->frameResources[this->currentFrame].imageAvailableSemaphore,
        {},
        &this->currentImageIndex);
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
      this->frameResources[this->currentFrame].framebuffer,
      this->swapchainImageViews[this->currentImageIndex],
      this->frameResources[this->currentFrame].depthImageView);

  vk::CommandBufferBeginInfo beginInfo{
      vk::CommandBufferUsageFlagBits::eSimultaneousUse, // flags
      nullptr,                                          // pInheritanceInfo
  };

  auto &commandBuffer = this->frameResources[this->currentFrame].commandBuffer;

  commandBuffer.begin(beginInfo);

  if (Context::get().presentQueue != Context::get().graphicsQueue) {
    vk::ImageMemoryBarrier barrierFromPresentToDraw = {
        vk::AccessFlagBits::eMemoryRead,                // srcAccessMask
        vk::AccessFlagBits::eMemoryRead,                // dstAccessMask
        vk::ImageLayout::eUndefined,                    // oldLayout
        vk::ImageLayout::eColorAttachmentOptimal,       // newLayout
        Context::get().presentQueueFamilyIndex,         // srcQueueFamilyIndex
        Context::get().graphicsQueueFamilyIndex,        // dstQueueFamilyIndex
        this->swapchainImages[this->currentImageIndex], // image
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

  std::array<vk::ClearValue, 2> clearValues{
      vk::ClearValue{std::array<float, 4>{1.0f, 0.8f, 0.4f, 0.0f}},
      vk::ClearValue{vk::ClearDepthStencilValue{1.0f, 0}},
  };

  vk::RenderPassBeginInfo renderPassBeginInfo{
      this->renderPass,                                     // renderPass
      this->frameResources[this->currentFrame].framebuffer, // framebuffer
      {{0, 0}, this->swapchainExtent},                      // renderArea
      static_cast<uint32_t>(clearValues.size()),            // clearValueCount
      clearValues.data(),                                   // pClearValues
  };

  commandBuffer.beginRenderPass(
      renderPassBeginInfo, vk::SubpassContents::eInline);

  vk::Viewport viewport{
      0.0f,                                             // x
      0.0f,                                             // y
      static_cast<float>(this->swapchainExtent.width),  // width
      static_cast<float>(this->swapchainExtent.height), // height
      0.0f,                                             // minDepth
      1.0f,                                             // maxDepth
  };

  vk::Rect2D scissor{{0, 0}, this->swapchainExtent};
  commandBuffer.setViewport(0, viewport);

  commandBuffer.setScissor(0, scissor);

  // Draw
  {
    vkr::CommandBuffer cmdBuffer{commandBuffer};
    drawFunction(cmdBuffer);
  }

  // End
  commandBuffer.endRenderPass();

  if (Context::get().presentQueue != Context::get().graphicsQueue) {
    vk::ImageMemoryBarrier barrierFromDrawToPresent{
        vk::AccessFlagBits::eMemoryRead,                // srcAccessMask
        vk::AccessFlagBits::eMemoryRead,                // dstAccessMask
        vk::ImageLayout::eColorAttachmentOptimal,       // oldLayout
        vk::ImageLayout::ePresentSrcKHR,                // newLayout
        Context::get().graphicsQueueFamilyIndex,        // srcQueueFamilyIndex
        Context::get().presentQueueFamilyIndex,         // dstQueueFamilyIndex
        this->swapchainImages[this->currentImageIndex], // image
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
      &this->frameResources[this->currentFrame]
           .imageAvailableSemaphore, // pWaitSemaphores
      &waitDstStageMask,             // pWaitDstStageMask
      1,                             // commandBufferCount
      &commandBuffer,                // pCommandBuffers
      1,                             // signalSemaphoreCount
      &this->frameResources[this->currentFrame]
           .renderingFinishedSemaphore, // pSignalSemaphores
  };

  Context::get().graphicsQueue.submit(
      submitInfo, this->frameResources[this->currentFrame].fence);

  vk::PresentInfoKHR presentInfo{
      1, // waitSemaphoreCount
      &this->frameResources[this->currentFrame]
           .renderingFinishedSemaphore, // pWaitSemaphores
      1,                                // swapchainCount
      &this->swapchain,                 // pSwapchains
      &this->currentImageIndex,         // pImageIndices
      nullptr,                          // pResults
  };

  try {
    Context::get().presentQueue.presentKHR(presentInfo);
  } catch (const vk::OutOfDateKHRError &e) {
    this->updateSize();
  }

  this->currentFrame = (this->currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;

  this->deltaTicks = SDL_GetTicks() - this->lastTicks;
}

void Window::updateSize() {
  Context::getDevice().waitIdle();

  this->destroyResizables();

  this->createSwapchain(this->getWidth(), this->getHeight());
  this->createSwapchainImageViews();
  this->createDepthResources();
  this->createRenderPass();
  this->allocateGraphicsCommandBuffers();
}

uint32_t Window::getWidth() const {
  int width;
  SDL_GetWindowSize(this->window, &width, nullptr);
  return static_cast<uint32_t>(width);
}

uint32_t Window::getHeight() const {
  int height;
  SDL_GetWindowSize(this->window, nullptr, &height);
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

float Window::getDelta() const { return (float)this->deltaTicks / 1000.0f; }

bool Window::getShouldClose() const { return this->shouldClose; }

void Window::setShouldClose(bool shouldClose) {
  this->shouldClose = shouldClose;
}

void Window::initVulkanExtensions() const {
  uint32_t sdlExtensionCount = 0;
  SDL_Vulkan_GetInstanceExtensions(this->window, &sdlExtensionCount, nullptr);
  std::vector<const char *> sdlExtensions(sdlExtensionCount);
  SDL_Vulkan_GetInstanceExtensions(
      this->window, &sdlExtensionCount, sdlExtensions.data());
  Window::requiredVulkanExtensions = sdlExtensions;
}

void Window::createVulkanSurface() {
  if (!SDL_Vulkan_CreateSurface(
          this->window,
          static_cast<VkInstance>(Context::get().instance),
          reinterpret_cast<VkSurfaceKHR *>(&this->surface))) {
    throw std::runtime_error(
        "Failed to create window surface: " + std::string(SDL_GetError()));
  }
}

void Window::createSyncObjects() {
  for (auto &resources : this->frameResources) {
    resources.imageAvailableSemaphore =
        Context::getDevice().createSemaphore({});
    resources.renderingFinishedSemaphore =
        Context::getDevice().createSemaphore({});
    resources.fence =
        Context::getDevice().createFence({vk::FenceCreateFlagBits::eSignaled});
  }
}

void Window::createSwapchain(uint32_t width, uint32_t height) {
  for (const auto &imageView : this->swapchainImageViews) {
    if (imageView) {
      Context::getDevice().destroy(imageView);
    }
  }
  this->swapchainImageViews.clear();

  auto surfaceCapabilities =
      Context::get().physicalDevice.getSurfaceCapabilitiesKHR(this->surface);
  auto surfaceFormats =
      Context::get().physicalDevice.getSurfaceFormatsKHR(this->surface);
  auto presentModes =
      Context::get().physicalDevice.getSurfacePresentModesKHR(this->surface);

  auto desiredNumImages = getSwapchainNumImages(surfaceCapabilities);
  auto desiredFormat = getSwapchainFormat(surfaceFormats);
  auto desiredExtent = getSwapchainExtent(width, height, surfaceCapabilities);
  auto desiredUsage = getSwapchainUsageFlags(surfaceCapabilities);
  auto desiredTransform = getSwapchainTransform(surfaceCapabilities);
  auto desiredPresentMode = getSwapchainPresentMode(presentModes);

  vk::SwapchainKHR oldSwapchain = this->swapchain;

  vk::SwapchainCreateInfoKHR createInfo{
      {}, // flags
      this->surface,
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

  this->swapchain = Context::getDevice().createSwapchainKHR(createInfo);

  if (oldSwapchain) {
    Context::getDevice().destroy(oldSwapchain);
  }

  this->swapchainImageFormat = desiredFormat.format;
  this->swapchainExtent = desiredExtent;

  this->swapchainImages =
      Context::getDevice().getSwapchainImagesKHR(this->swapchain);
}

void Window::createSwapchainImageViews() {
  this->swapchainImageViews.resize(this->swapchainImages.size());

  for (size_t i = 0; i < swapchainImages.size(); i++) {
    vk::ImageViewCreateInfo createInfo{
        {},
        this->swapchainImages[i],
        vk::ImageViewType::e2D,
        this->swapchainImageFormat,
        {
            vk::ComponentSwizzle::eIdentity, // r
            vk::ComponentSwizzle::eIdentity, // g
            vk::ComponentSwizzle::eIdentity, // b
            vk::ComponentSwizzle::eIdentity, // a
        },
        {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1}};

    this->swapchainImageViews[i] =
        Context::getDevice().createImageView(createInfo);
  }
}

void Window::allocateGraphicsCommandBuffers() {
  auto commandBuffers = Context::getDevice().allocateCommandBuffers(
      {Context::get().graphicsCommandPool,
       vk::CommandBufferLevel::ePrimary,
       MAX_FRAMES_IN_FLIGHT});

  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    this->frameResources[i].commandBuffer = commandBuffers[i];
  }
}

void Window::createDepthResources() {
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
      this->depthImageFormat = format;
      validDepthFormat = true;
      break;
    }
  }
  assert(validDepthFormat);

  for (auto &resources : this->frameResources) {
    vk::ImageCreateInfo imageCreateInfo{
        {},
        vk::ImageType::e2D,
        this->depthImageFormat,
        {
            this->swapchainExtent.width,
            this->swapchainExtent.height,
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

    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

    if (vmaCreateImage(
            Context::get().allocator,
            reinterpret_cast<VkImageCreateInfo *>(&imageCreateInfo),
            &allocInfo,
            reinterpret_cast<VkImage *>(&resources.depthImage),
            &resources.depthImageAllocation,
            nullptr) != VK_SUCCESS) {
      throw std::runtime_error("Failed to create one of the depth images");
    }

    vk::ImageViewCreateInfo imageViewCreateInfo{
        {},
        resources.depthImage,
        vk::ImageViewType::e2D,
        this->depthImageFormat,
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

    resources.depthImageView =
        Context::getDevice().createImageView(imageViewCreateInfo);
  }
}

void Window::createRenderPass() {
  std::array<vk::AttachmentDescription, 2> attachmentDescriptions{
      vk::AttachmentDescription{
          {},                               // flags
          this->swapchainImageFormat,       // format
          vk::SampleCountFlagBits::e1,      // samples
          vk::AttachmentLoadOp::eClear,     // loadOp
          vk::AttachmentStoreOp::eStore,    // storeOp
          vk::AttachmentLoadOp::eDontCare,  // stencilLoadOp
          vk::AttachmentStoreOp::eDontCare, // stencilStoreOp
          vk::ImageLayout::eUndefined,      // initialLayout
          vk::ImageLayout::ePresentSrcKHR,  // finalLayout (TODO: maybe change
                                            // this?)
      },
      vk::AttachmentDescription{
          {},                                              // flags
          this->depthImageFormat,                          // format
          vk::SampleCountFlagBits::e1,                     // samples
          vk::AttachmentLoadOp::eClear,                    // loadOp
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
          1,                                               // attachment
          vk::ImageLayout::eDepthStencilAttachmentOptimal, // layout
      },
  };

  std::array<vk::SubpassDescription, 1> subpassDescriptions{
      vk::SubpassDescription{
          {},                               // flags
          vk::PipelineBindPoint::eGraphics, // pipelineBindPoint
          0,                                // inputAttachmentCount
          nullptr,                          // pInputAttachments
          1,                                // colorAttachmentCount
          colorAttachmentReferences.data(), // pColorAttachments
          nullptr,                          // pResolveAttachments
          depthAttachmentReferences.data(), // pDepthStencilAttachment
          0,                                // preserveAttachmentCount
          nullptr,                          // pPreserveAttachments
      },
  };

  std::array<vk::SubpassDependency, 1> dependencies{
      vk::SubpassDependency{
          0,                                                 // srcSubpass
          VK_SUBPASS_EXTERNAL,                               // dstSubpass
          vk::PipelineStageFlagBits::eColorAttachmentOutput, // srcStageMask
          vk::PipelineStageFlagBits::eFragmentShader,        // dstStageMask
          vk::AccessFlagBits::eColorAttachmentWrite,         // srcAccessMask
          vk::AccessFlagBits::eShaderRead,                   // dstAccessMask
          {},                                                // dependencyFlags
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

  this->renderPass =
      Context::getDevice().createRenderPass(renderPassCreateInfo);
}

void Window::regenFramebuffer(
    vk::Framebuffer &framebuffer,
    vk::ImageView colorImageView,
    vk::ImageView depthImageView) {
  Context::getDevice().destroy(framebuffer);

  std::array<vk::ImageView, 2> attachments{
      colorImageView,
      depthImageView,
  };

  vk::FramebufferCreateInfo createInfo = {
      {},                                        // flags
      this->renderPass,                          // renderPass
      static_cast<uint32_t>(attachments.size()), // attachmentCount
      attachments.data(),                        // pAttachments
      this->swapchainExtent.width,               // width
      this->swapchainExtent.height,              // height
      1,                                         // layers
  };

  framebuffer = Context::getDevice().createFramebuffer(createInfo);
}

void Window::destroyResizables() {
  Context::getDevice().waitIdle();

  for (auto &resources : this->frameResources) {
    Context::getDevice().freeCommandBuffers(
        Context::get().graphicsCommandPool, resources.commandBuffer);

    if (resources.depthImage) {
      Context::getDevice().destroy(resources.depthImageView);
      vmaDestroyImage(
          Context::get().allocator,
          resources.depthImage,
          resources.depthImageAllocation);
      resources.depthImage = nullptr;
      resources.depthImageAllocation = VK_NULL_HANDLE;
    }
  }

  Context::getDevice().destroy(this->renderPass);
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

  log::fatal(
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
      return presentMode;
    }
  }

  for (const auto &presentMode : presentModes) {
    if (presentMode == vk::PresentModeKHR::eMailbox) {
      return presentMode;
    }
  }

  for (const auto &presentMode : presentModes) {
    if (presentMode == vk::PresentModeKHR::eFifo) {
      return presentMode;
    }
  }

  log::fatal("FIFO present mode is not supported by the swapchain!");

  return static_cast<vk::PresentModeKHR>(-1);
}
