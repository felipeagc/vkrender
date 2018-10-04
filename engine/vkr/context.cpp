#include "context.hpp"
#include "commandbuffer.hpp"
#include "window.hpp"
#include <iostream>

using namespace vkr;

// Debug callback

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugReportFlagsEXT flags,
    VkDebugReportObjectTypeEXT objType,
    uint64_t obj,
    size_t location,
    int32_t code,
    const char *layerPrefix,
    const char *msg,
    void *userData) {
  std::cerr << "Validation layer: " << msg << "\n";

  return VK_FALSE;
}

VkResult CreateDebugReportCallbackEXT(
    VkInstance instance,
    const VkDebugReportCallbackCreateInfoEXT *pCreateInfo,
    const VkAllocationCallbacks *pAllocator,
    VkDebugReportCallbackEXT *pCallback) {
  auto func = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(
      instance, "vkCreateDebugReportCallbackEXT");
  if (func != nullptr) {
    return func(instance, pCreateInfo, pAllocator, pCallback);
  } else {
    return VK_ERROR_EXTENSION_NOT_PRESENT;
  }
}

void DestroyDebugReportCallbackEXT(
    VkInstance instance,
    VkDebugReportCallbackEXT callback,
    const VkAllocationCallbacks *pAllocator) {
  auto func = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(
      instance, "vkDestroyDebugReportCallbackEXT");
  if (func != nullptr) {
    func(instance, callback, pAllocator);
  }
}

Context::Context(const Window &window): window(window) {
  this->createInstance(window.getVulkanExtensions());
#ifndef NDEBUG
  this->setupDebugCallback();
#endif
  this->surface = window.createVulkanSurface(this->instance);

  this->createDevice();

  this->getDeviceQueues();

  this->setupMemoryAllocator();

  this->createSyncObjects();

  this->createSwapchain(window.getWidth(), window.getHeight());
  this->createSwapchainImageViews();

  this->createGraphicsCommandPool();
  this->createTransientCommandPool();
  this->allocateGraphicsCommandBuffers();

  this->createDepthResources();

  this->createRenderPass();
}

Context::~Context() {
  this->device.waitIdle();

  this->destroyResizables();

  this->device.destroy(transientCommandPool);
  this->device.destroy(graphicsCommandPool);

  for (auto &swapchainImageView : this->swapchainImageViews) {
    this->device.destroy(swapchainImageView);
  }

  this->device.destroy(this->swapchain);

  for (auto &frameResource : this->frameResources) {
    this->device.destroy(frameResource.framebuffer);
    this->device.destroy(frameResource.imageAvailableSemaphore);
    this->device.destroy(frameResource.renderingFinishedSemaphore);
    this->device.destroy(frameResource.fence);
  }

  if (this->allocator != VK_NULL_HANDLE) {
    vmaDestroyAllocator(this->allocator);
  }

  this->device.destroy();

  instance.destroy(this->surface);

  if (this->callback) {
    DestroyDebugReportCallbackEXT(this->instance, this->callback, nullptr);
  }

  instance.destroy();
}


void Context::present(std::function<void(CommandBuffer&)> drawFunction) {
  // Begin
  this->device.waitForFences(this->frameResources[this->currentFrame].fence, VK_TRUE, UINT64_MAX);

  this->device.resetFences(this->frameResources[this->currentFrame].fence);

  try {
    this->device.acquireNextImageKHR(
        this->swapchain,
        UINT64_MAX,
        this->frameResources[this->currentFrame].imageAvailableSemaphore,
        {},
        &this->currentImageIndex);
  } catch (const vk::OutOfDateKHRError &e) {
    this->updateSize(this->window.getWidth(), this->window.getHeight());
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

  if (this->presentQueue != this->graphicsQueue) {
    vk::ImageMemoryBarrier barrierFromPresentToDraw = {
        vk::AccessFlagBits::eMemoryRead,   // srcAccessMask
        vk::AccessFlagBits::eMemoryRead,   // dstAccessMask
        vk::ImageLayout::eUndefined,       // oldLayout
        vk::ImageLayout::ePresentSrcKHR,   // newLayout
        this->presentQueueFamilyIndex,     // srcQueueFamilyIndex
        this->graphicsQueueFamilyIndex,    // dstQueueFamilyIndex
        this->swapchainImages[this->currentImageIndex], // image
        imageSubresourceRange,             // subresourceRange
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
      vk::ClearValue{{1.0f, 0}},
  };

  vk::RenderPassBeginInfo renderPassBeginInfo{
    this->renderPass, // renderPass
    this->frameResources[this->currentFrame].framebuffer, // framebuffer
    {{0, 0}, this->swapchainExtent}, // renderArea
    static_cast<uint32_t>(clearValues.size()), // clearValueCount
    clearValues.data(), // pClearValues
  };

  commandBuffer.beginRenderPass(
      renderPassBeginInfo, vk::SubpassContents::eInline);

  vk::Viewport viewport{
    0.0f, // x
    0.0f, // y
    static_cast<float>(this->swapchainExtent.width), // width
    static_cast<float>(this->swapchainExtent.height), // height
    0.0f, // minDepth
    1.0f, // maxDepth
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

  if (this->presentQueue != this->graphicsQueue) {
    vk::ImageMemoryBarrier barrierFromDrawToPresent{
        vk::AccessFlagBits::eMemoryRead,                // srcAccessMask
        vk::AccessFlagBits::eMemoryRead,                // dstAccessMask
        vk::ImageLayout::ePresentSrcKHR,                // oldLayout
        vk::ImageLayout::ePresentSrcKHR,                // newLayout
        this->graphicsQueueFamilyIndex,                 // srcQueueFamilyIndex
        this->presentQueueFamilyIndex,                  // dstQueueFamilyIndex
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
      &commandBuffer, // pCommandBuffers
      1,                   // signalSemaphoreCount
      &this->frameResources[this->currentFrame]
           .renderingFinishedSemaphore, // pSignalSemaphores
  };

  this->graphicsQueue.submit(
      submitInfo, this->frameResources[this->currentFrame].fence);

  vk::PresentInfoKHR presentInfo{
    1, // waitSemaphoreCount
      &this->frameResources[this->currentFrame].renderingFinishedSemaphore, // pWaitSemaphores
      1, // swapchainCount
      &this->swapchain, // pSwapchains
      &this->currentImageIndex, // pImageIndices
      nullptr, // pResults
  };

  try {
    this->presentQueue.presentKHR(presentInfo);
  } catch(const vk::OutOfDateKHRError &e) {
    this->updateSize(this->window.getWidth(), this->window.getHeight());
  }

  this->currentFrame = (this->currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void Context::updateSize(uint32_t width, uint32_t height) {
  this->device.waitIdle();

  this->destroyResizables();

  this->createSwapchain(this->window.getWidth(), this->window.getHeight());
  this->createSwapchainImageViews();
  this->createDepthResources();
  this->createRenderPass();
  this->allocateGraphicsCommandBuffers();
}


void Context::createInstance(std::vector<const char *> sdlExtensions) {
#ifndef NDEBUG
  if (!checkValidationLayerSupport()) {
    throw std::runtime_error("Validation layers requested, but not available");
  }
#endif

  vk::ApplicationInfo appInfo{"App",
                              VK_MAKE_VERSION(1, 0, 0),
                              "No engine",
                              VK_MAKE_VERSION(1, 0, 0),
                              VK_API_VERSION_1_0};

  vk::InstanceCreateInfo createInfo{{}, &appInfo};

#ifdef NDEBUG
  createInfo.enabledLayerCount = 0;
  createInfo.ppEnabledLayerNames = nullptr;
#else
  createInfo.enabledLayerCount =
      static_cast<uint32_t>(REQUIRED_VALIDATION_LAYERS.size());
  createInfo.ppEnabledLayerNames = REQUIRED_VALIDATION_LAYERS.data();
#endif

  // Set required instance extensions
  auto extensions = this->getRequiredExtensions(sdlExtensions);
  createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
  createInfo.ppEnabledExtensionNames = extensions.data();

  this->instance = vk::createInstance(createInfo);
}

void Context::createDevice() {
  auto physicalDevices = this->instance.enumeratePhysicalDevices();

  for (auto physicalDevice : physicalDevices) {
    if (checkPhysicalDeviceProperties(
            physicalDevice,
            &graphicsQueueFamilyIndex,
            &presentQueueFamilyIndex,
            &transferQueueFamilyIndex)) {
      this->physicalDevice = physicalDevice;
      break;
    }
  }

  if (!physicalDevice) {
    throw std::runtime_error(
        "Could not select physical device based on chosen properties");
  }

  std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
  std::array<float, 1> queuePriorities = {1.0f};

  queueCreateInfos.push_back({
      {},
      graphicsQueueFamilyIndex,
      static_cast<uint32_t>(queuePriorities.size()),
      queuePriorities.data(),
  });

  if (presentQueueFamilyIndex != graphicsQueueFamilyIndex) {
    queueCreateInfos.push_back({
        {},
        presentQueueFamilyIndex,
        static_cast<uint32_t>(queuePriorities.size()),
        queuePriorities.data(),
    });
  }

  if (transferQueueFamilyIndex != graphicsQueueFamilyIndex) {
    queueCreateInfos.push_back({
        {},
        transferQueueFamilyIndex,
        static_cast<uint32_t>(queuePriorities.size()),
        queuePriorities.data(),
    });
  }

  vk::DeviceCreateInfo deviceCreateInfo{
      {},
      static_cast<uint32_t>(queueCreateInfos.size()),
      queueCreateInfos.data()};

  // Validation layer stuff
#ifdef NDEBUG
  deviceCreateInfo.enabledLayerCount = 0;
  deviceCreateInfo.ppEnabledLayerNames = nullptr;
#else
  deviceCreateInfo.enabledLayerCount =
      static_cast<uint32_t>(REQUIRED_VALIDATION_LAYERS.size());
  deviceCreateInfo.ppEnabledLayerNames = REQUIRED_VALIDATION_LAYERS.data();
#endif

  deviceCreateInfo.enabledExtensionCount =
      static_cast<uint32_t>(REQUIRED_DEVICE_EXTENSIONS.size());
  deviceCreateInfo.ppEnabledExtensionNames = REQUIRED_DEVICE_EXTENSIONS.data();

  deviceCreateInfo.pEnabledFeatures = nullptr;

  this->device = this->physicalDevice.createDevice(deviceCreateInfo);
}

void Context::getDeviceQueues() {
  this->device.getQueue(this->graphicsQueueFamilyIndex, 0, &this->graphicsQueue);
  this->device.getQueue(this->presentQueueFamilyIndex, 0, &this->presentQueue);
  this->device.getQueue(this->transferQueueFamilyIndex, 0, &this->transferQueue);
}

void Context::setupMemoryAllocator() {
    VmaAllocatorCreateInfo allocatorInfo = {
      .physicalDevice = this->physicalDevice,
      .device = this->device,
  };

  vmaCreateAllocator(&allocatorInfo, &this->allocator);
}

void Context::createSyncObjects() {
  for (auto &resources : this->frameResources) {
    resources.imageAvailableSemaphore =
      this->device.createSemaphore({});
    resources.renderingFinishedSemaphore =
      this->device.createSemaphore({});
    resources.fence = this->device.createFence({vk::FenceCreateFlagBits::eSignaled});
  }
}

void Context::createSwapchain(uint32_t width, uint32_t height) {
  for (const auto &imageView : this->swapchainImageViews) {
    if (imageView) {
      this->device.destroy(imageView);
    }
  }
  this->swapchainImageViews.clear();

  auto surfaceCapabilities = this->physicalDevice.getSurfaceCapabilitiesKHR(this->surface);
  auto surfaceFormats = this->physicalDevice.getSurfaceFormatsKHR(this->surface);
  auto presentModes = this->physicalDevice.getSurfacePresentModesKHR(this->surface);

  auto desiredNumImages = getSwapchainNumImages(surfaceCapabilities);
  auto desiredFormat = getSwapchainFormat(surfaceFormats);
  auto desiredExtent =
      getSwapchainExtent(width, height, surfaceCapabilities);
  auto desiredUsage = getSwapchainUsageFlags(surfaceCapabilities);
  auto desiredTransform =
      getSwapchainTransform(surfaceCapabilities);
  auto desiredPresentMode = getSwapchainPresentMode(presentModes);

  vk::SwapchainKHR oldSwapchain = this->swapchain;

  vk::SwapchainCreateInfoKHR createInfo{
    {}, // flags
    this->surface,
    desiredNumImages, // minImageCount
    desiredFormat.format, // imageFormat
    desiredFormat.colorSpace, // imageColorSpace
    desiredExtent, // imageExtent
    1, // imageArrayLayers
    desiredUsage, // imageUsage
    vk::SharingMode::eExclusive, // imageSharingMode
    0, // queueFamilyIndexCount
    nullptr, // pQueueFamiylIndices
    desiredTransform, // preTransform
    vk::CompositeAlphaFlagBitsKHR::eOpaque, // compositeAlpha
    desiredPresentMode, // presentMode
    VK_TRUE, // clipped
    oldSwapchain // oldSwapchain
  };

  this->swapchain = this->device.createSwapchainKHR(createInfo);

  if (oldSwapchain) {
    this->device.destroy(oldSwapchain);
  }

  this->swapchainImageFormat = desiredFormat.format;
  this->swapchainExtent = desiredExtent;

  this->swapchainImages = this->device.getSwapchainImagesKHR(this->swapchain);
}

void Context::createSwapchainImageViews() {
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

    this->swapchainImageViews[i] = this->device.createImageView(createInfo);
  }
}

void Context::createGraphicsCommandPool() {
  this->graphicsCommandPool = this->device.createCommandPool(
      {vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
       this->graphicsQueueFamilyIndex});
}

void Context::createTransientCommandPool() {
  this->transientCommandPool =
      this->device.createCommandPool({vk::CommandPoolCreateFlagBits::eTransient,
                                      this->graphicsQueueFamilyIndex});
}

void Context::allocateGraphicsCommandBuffers() {
  auto commandBuffers =
      this->device.allocateCommandBuffers({this->graphicsCommandPool,
                                           vk::CommandBufferLevel::ePrimary,
                                           MAX_FRAMES_IN_FLIGHT});

  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    this->frameResources[i].commandBuffer = commandBuffers[i];
  }
}

void Context::createDepthResources() {
  for (auto &resources : this->frameResources) {
    this->depthImageFormat = vk::Format::eD16Unorm; // TODO: change this?
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
            this->allocator,
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
            vk::ImageAspectFlagBits::eDepth, // aspectMask
            0,                               // baseMipLevel
            1,                               // levelCount
            0,                               // baseArrayLayer
            1,                               // layerCount
        },                                   // subresourceRange
    };

    resources.depthImageView =
        this->device.createImageView(imageViewCreateInfo);
  }
}

void Context::createRenderPass() {
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
          {},                                            // flags
          this->depthImageFormat,                        // format
          vk::SampleCountFlagBits::e1,                   // samples
          vk::AttachmentLoadOp::eClear,                  // loadOp
          vk::AttachmentStoreOp::eStore,                 // storeOp
          vk::AttachmentLoadOp::eDontCare,               // stencilLoadOp
          vk::AttachmentStoreOp::eDontCare,              // stencilStoreOp
          vk::ImageLayout::eUndefined,                   // initialLayout
          vk::ImageLayout::eDepthStencilReadOnlyOptimal, // finalLayout
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

  this->renderPass = this->device.createRenderPass(renderPassCreateInfo);
}


void Context::regenFramebuffer(
    vk::Framebuffer &framebuffer,
    vk::ImageView colorImageView,
    vk::ImageView depthImageView) {
  this->device.destroy(framebuffer);

  std::array<vk::ImageView, 2> attachments{
      colorImageView,
      depthImageView,
  };

  vk::FramebufferCreateInfo createInfo = {
    {}, // flags
    this->renderPass, // renderPass
    static_cast<uint32_t>(attachments.size()), // attachmentCount
    attachments.data(), // pAttachments
    this->swapchainExtent.width, // width
    this->swapchainExtent.height, // height
    1, // layers
  };

  framebuffer = this->device.createFramebuffer(createInfo);
}

void Context::destroyResizables() {
  this->device.waitIdle();

  for (auto &resources : this->frameResources) {
    this->device.freeCommandBuffers(this->graphicsCommandPool, resources.commandBuffer);

    if (resources.depthImage) {
      this->device.destroy(resources.depthImageView);
      vmaDestroyImage(
          this->allocator,
          resources.depthImage,
          resources.depthImageAllocation);
      resources.depthImage = nullptr;
      resources.depthImageAllocation = VK_NULL_HANDLE;
    }
  }

  this->device.destroy(this->renderPass);
}

// Misc

std::vector<const char *>
Context::getRequiredExtensions(std::vector<const char *> sdlExtensions) {
  std::vector<const char *> extensions{sdlExtensions};

#ifndef NDEBUG
  extensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
#endif

  return extensions;
}

bool Context::checkValidationLayerSupport() {
  auto availableLayers = vk::enumerateInstanceLayerProperties();

  for (const char *layerName : REQUIRED_VALIDATION_LAYERS) {
    bool layerFound = false;

    for (const auto &layerProperties : availableLayers) {
      if (strcmp(layerProperties.layerName, layerName) == 0) {
        layerFound = true;
        break;
      }
    }

    if (!layerFound) {
      return false;
    }
  }

  return true;
}

void Context::setupDebugCallback() {
  vk::DebugReportCallbackCreateInfoEXT createInfo{
      vk::DebugReportFlagBitsEXT::eError | vk::DebugReportFlagBitsEXT::eWarning,
      debugCallback};

  if (CreateDebugReportCallbackEXT(
          this->instance,
          reinterpret_cast<VkDebugReportCallbackCreateInfoEXT *>(&createInfo),
          nullptr,
          reinterpret_cast<VkDebugReportCallbackEXT *>(&this->callback)) !=
      VK_SUCCESS) {

    throw std::runtime_error("Failed to setup debug callback");
  }
}

bool Context::checkPhysicalDeviceProperties(
    vk::PhysicalDevice physicalDevice,
    uint32_t *graphicsQueueFamily,
    uint32_t *presentQueueFamily,
    uint32_t *transferQueueFamily) {
  auto availableExtensions =
      physicalDevice.enumerateDeviceExtensionProperties();

  for (const auto &requiredExtension : REQUIRED_DEVICE_EXTENSIONS) {
    bool found = false;
    for (const auto &extension : availableExtensions) {
      if (strcmp(requiredExtension, extension.extensionName) == 0) {
        found = true;
      }
    }

    if (!found) {
      std::cout << "Physical device " << physicalDevice
                << " doesn't support extension named \"" << requiredExtension
                << "\"" << std::endl;
      return false;
    }
  }

  auto deviceProperties = physicalDevice.getProperties();
  auto deviceFeatures = physicalDevice.getFeatures();

  uint32_t majorVersion = VK_VERSION_MAJOR(deviceProperties.apiVersion);
  // uint32_t minorVersion = VK_VERSION_MINOR(deviceProperties.apiVersion);
  // uint32_t patchVersion = VK_VERSION_PATCH(deviceProperties.apiVersion);

  if (majorVersion < 1 && deviceProperties.limits.maxImageDimension2D < 4096) {
    std::cout << "Physical device " << physicalDevice
              << " doesn't support required parameters!" << std::endl;
    return false;
  }

  auto queueFamilyProperties = physicalDevice.getQueueFamilyProperties();
  std::vector<VkBool32> queuePresentSupport(queueFamilyProperties.size());
  std::vector<VkBool32> queueTransferSupport(queueFamilyProperties.size());

  uint32_t graphicsQueueFamilyIndex = UINT32_MAX;
  uint32_t presentQueueFamilyIndex = UINT32_MAX;
  uint32_t transferQueueFamilyIndex = UINT32_MAX;

  // TODO: figure out better logic to single out a transfer queue

  for (uint32_t i = 0; i < queueFamilyProperties.size(); i++) {
    queuePresentSupport[i] =
        physicalDevice.getSurfaceSupportKHR(i, this->surface);

    if (queueFamilyProperties[i].queueCount > 0 &&
        queueFamilyProperties[i].queueFlags & vk::QueueFlagBits::eTransfer &&
        !(queueFamilyProperties[i].queueFlags & vk::QueueFlagBits::eGraphics)) {
      transferQueueFamilyIndex = i;
    }

    if (queueFamilyProperties[i].queueCount > 0 &&
        queueFamilyProperties[i].queueFlags & vk::QueueFlagBits::eGraphics) {
      if (graphicsQueueFamilyIndex == UINT32_MAX) {
        graphicsQueueFamilyIndex = i;
      }

      if (queuePresentSupport[i]) {
        if (transferQueueFamilyIndex == UINT32_MAX) {
          *transferQueueFamily = i;
        }
        *graphicsQueueFamily = i;
        *presentQueueFamily = i;
        return true;
      }
    }
  }

  if (transferQueueFamilyIndex == UINT32_MAX) {
    transferQueueFamilyIndex = graphicsQueueFamilyIndex;
  }

  for (uint32_t i = 0; i < queueFamilyProperties.size(); i++) {
    if (queuePresentSupport[i]) {
      presentQueueFamilyIndex = i;
      break;
    }
  }

  if (graphicsQueueFamilyIndex == UINT32_MAX ||
      presentQueueFamilyIndex == UINT32_MAX ||
      transferQueueFamilyIndex == UINT32_MAX) {
    std::cout << "Could not find queue family with requested properties on "
                 "physical device "
              << physicalDevice << std::endl;
    return false;
  }

  *graphicsQueueFamily = graphicsQueueFamilyIndex;
  *presentQueueFamily = presentQueueFamilyIndex;
  *transferQueueFamily = transferQueueFamilyIndex;

  return true;
}

// Swapchain misc functions

uint32_t Context::getSwapchainNumImages(
    const vk::SurfaceCapabilitiesKHR &surfaceCapabilities) {
  uint32_t imageCount = surfaceCapabilities.minImageCount + 1;

  if (surfaceCapabilities.maxImageCount > 0 &&
      imageCount > surfaceCapabilities.maxImageCount) {
    imageCount = surfaceCapabilities.maxImageCount;
  }

  return imageCount;
}

vk::SurfaceFormatKHR
Context::getSwapchainFormat(const std::vector<vk::SurfaceFormatKHR> &formats) {
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

vk::Extent2D Context::getSwapchainExtent(
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

vk::ImageUsageFlags Context::getSwapchainUsageFlags(
    const vk::SurfaceCapabilitiesKHR &surfaceCapabilities) {
  if (surfaceCapabilities.supportedUsageFlags &
      vk::ImageUsageFlagBits::eTransferDst) {
    return vk::ImageUsageFlagBits::eColorAttachment |
           vk::ImageUsageFlagBits::eTransferDst;
  }
  std::cout << "VK_IMAGE_USAGE_TRANSFER_DST image usage is not supported by "
               "the swap chain!"
            << std::endl
            << "Supported swap chain's image usages include:" << std::endl
            << (surfaceCapabilities.supportedUsageFlags &
                        vk::ImageUsageFlagBits::eTransferSrc
                    ? "    vk::ImageUsageFlagBits::TRANSFER_SRC\n"
                    : "")
            << (surfaceCapabilities.supportedUsageFlags &
                        vk::ImageUsageFlagBits::eTransferDst
                    ? "    vk::ImageUsageFlagBits::TRANSFER_DST\n"
                    : "")
            << (surfaceCapabilities.supportedUsageFlags &
                        vk::ImageUsageFlagBits::eSampled
                    ? "    vk::ImageUsageFlagBits::SAMPLED\n"
                    : "")
            << (surfaceCapabilities.supportedUsageFlags &
                        vk::ImageUsageFlagBits::eStorage
                    ? "    vk::ImageUsageFlagBits::STORAGE\n"
                    : "")
            << (surfaceCapabilities.supportedUsageFlags &
                        vk::ImageUsageFlagBits::eColorAttachment
                    ? "    vk::ImageUsageFlagBits::COLOR_ATTACHMENT\n"
                    : "")
            << (surfaceCapabilities.supportedUsageFlags &
                        vk::ImageUsageFlagBits::eDepthStencilAttachment
                    ? "    vk::ImageUsageFlagBits::DEPTH_STENCIL_ATTACHMENT\n"
                    : "")
            << (surfaceCapabilities.supportedUsageFlags &
                        vk::ImageUsageFlagBits::eTransientAttachment
                    ? "    vk::ImageUsageFlagBits::TRANSIENT_ATTACHMENT\n"
                    : "")
            << (surfaceCapabilities.supportedUsageFlags &
                        vk::ImageUsageFlagBits::eInputAttachment
                    ? "    vk::ImageUsageFlagBits::INPUT_ATTACHMENT"
                    : "")
            << std::endl;

  return static_cast<vk::ImageUsageFlags>(-1);
}

vk::SurfaceTransformFlagBitsKHR Context::getSwapchainTransform(
    const vk::SurfaceCapabilitiesKHR &surfaceCapabilities) {
  if (surfaceCapabilities.supportedTransforms &
      vk::SurfaceTransformFlagBitsKHR::eIdentity) {
    return vk::SurfaceTransformFlagBitsKHR::eIdentity;
  } else {
    return surfaceCapabilities.currentTransform;
  }
}

vk::PresentModeKHR Context::getSwapchainPresentMode(
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

  std::cout << "FIFO present mode is not supported by the swap chain!"
            << std::endl;

  return static_cast<vk::PresentModeKHR>(-1);
}
