#include "context.hpp"
#include "commandbuffer.hpp"
#include "window.hpp"
#include <fstl/logging.hpp>
#include <iostream>

using namespace vkr;

// Debug callback

// Ignore warnings for this function
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugReportFlagsEXT flags,
    VkDebugReportObjectTypeEXT objType,
    uint64_t obj,
    size_t location,
    int32_t code,
    const char *layerPrefix,
    const char *msg,
    void *userData) {
  fstl::log::error("Validation layer: {}", msg);

  return VK_FALSE;
}
#pragma GCC diagnostic pop

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

Context::Context() {
  this->createInstance();
#ifndef NDEBUG
  this->setupDebugCallback();
#endif
}

Context::~Context() {
  this->device_.waitIdle();

  this->descriptorManager_.destroy();

  this->device_.destroy(transientCommandPool_);
  this->device_.destroy(graphicsCommandPool_);

  if (this->allocator_ != VK_NULL_HANDLE) {
    vmaDestroyAllocator(this->allocator_);
  }

  this->device_.destroy();

  if (this->callback_) {
    DestroyDebugReportCallbackEXT(this->instance_, this->callback_, nullptr);
  }

  instance_.destroy();

  SDL_Quit();
}

void Context::createInstance() {
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
  auto extensions =
      this->getRequiredExtensions(Window::requiredVulkanExtensions);
  createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
  createInfo.ppEnabledExtensionNames = extensions.data();

  this->instance_ = vk::createInstance(createInfo);
}

void Context::lazyInit(vk::SurfaceKHR &surface) {
  if (this->device_) {
    return;
  }

  this->createDevice(surface);

  this->getDeviceQueues();

  this->setupMemoryAllocator();

  this->createGraphicsCommandPool();
  this->createTransientCommandPool();

  this->descriptorManager_.init();
}

void Context::createDevice(vk::SurfaceKHR &surface) {
  auto physicalDevices = this->instance_.enumeratePhysicalDevices();

  for (auto physicalDevice : physicalDevices) {
    if (checkPhysicalDeviceProperties(
            physicalDevice,
            surface,
            &graphicsQueueFamilyIndex_,
            &presentQueueFamilyIndex_,
            &transferQueueFamilyIndex_)) {
      this->physicalDevice_ = physicalDevice;
      break;
    }
  }

  if (!this->physicalDevice_) {
    throw std::runtime_error(
        "Could not select physical device based on chosen properties");
  }

  fstl::log::info(
      "Using physical device: {}",
      this->physicalDevice_.getProperties().deviceName);

  std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
  std::array<float, 1> queuePriorities = {1.0f};

  queueCreateInfos.push_back({
      {},
      graphicsQueueFamilyIndex_,
      static_cast<uint32_t>(queuePriorities.size()),
      queuePriorities.data(),
  });

  if (presentQueueFamilyIndex_ != graphicsQueueFamilyIndex_) {
    queueCreateInfos.push_back({
        {},
        presentQueueFamilyIndex_,
        static_cast<uint32_t>(queuePriorities.size()),
        queuePriorities.data(),
    });
  }

  if (transferQueueFamilyIndex_ != graphicsQueueFamilyIndex_) {
    queueCreateInfos.push_back({
        {},
        transferQueueFamilyIndex_,
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

  // Enable all features
  auto features = this->physicalDevice_.getFeatures();
  deviceCreateInfo.pEnabledFeatures = &features;

  this->device_ = this->physicalDevice_.createDevice(deviceCreateInfo);
}

void Context::getDeviceQueues() {
  this->device_.getQueue(
      this->graphicsQueueFamilyIndex_, 0, &this->graphicsQueue_);
  this->device_.getQueue(this->presentQueueFamilyIndex_, 0, &this->presentQueue_);
  this->device_.getQueue(
      this->transferQueueFamilyIndex_, 0, &this->transferQueue_);
}

void Context::setupMemoryAllocator() {
  VmaAllocatorCreateInfo allocatorInfo = {};
  allocatorInfo.physicalDevice = this->physicalDevice_;
  allocatorInfo.device = this->device_;

  vmaCreateAllocator(&allocatorInfo, &this->allocator_);
}

void Context::createGraphicsCommandPool() {
  this->graphicsCommandPool_ = this->device_.createCommandPool(
      {vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
       this->graphicsQueueFamilyIndex_});
}

void Context::createTransientCommandPool() {
  this->transientCommandPool_ =
      this->device_.createCommandPool({vk::CommandPoolCreateFlagBits::eTransient,
                                      this->graphicsQueueFamilyIndex_});
}

// Misc

vk::SampleCountFlagBits Context::getMaxUsableSampleCount() {
  auto properties = this->physicalDevice_.getProperties();
  vk::SampleCountFlags colorSamples =
      properties.limits.framebufferColorSampleCounts;
  vk::SampleCountFlags depthSamples =
      properties.limits.framebufferDepthSampleCounts;

  vk::SampleCountFlags counts = static_cast<vk::SampleCountFlags>(std::min(
      static_cast<unsigned int>(colorSamples),
      static_cast<unsigned int>(depthSamples)));

  if (counts & vk::SampleCountFlagBits::e64) {
    fstl::log::debug("Max samples: {}", 64);
    return vk::SampleCountFlagBits::e64;
  }
  if (counts & vk::SampleCountFlagBits::e32) {
    fstl::log::debug("Max samples: {}", 32);
    return vk::SampleCountFlagBits::e32;
  }
  if (counts & vk::SampleCountFlagBits::e16) {
    fstl::log::debug("Max samples: {}", 16);
    return vk::SampleCountFlagBits::e16;
  }
  if (counts & vk::SampleCountFlagBits::e8) {
    fstl::log::debug("Max samples: {}", 8);
    return vk::SampleCountFlagBits::e8;
  }
  if (counts & vk::SampleCountFlagBits::e4) {
    fstl::log::debug("Max samples: {}", 4);
    return vk::SampleCountFlagBits::e4;
  }
  if (counts & vk::SampleCountFlagBits::e2) {
    fstl::log::debug("Max samples: {}", 2);
    return vk::SampleCountFlagBits::e2;
  }

  fstl::log::debug("Max samples: {}", 1);

  return vk::SampleCountFlagBits::e1;
}

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
          this->instance_,
          reinterpret_cast<VkDebugReportCallbackCreateInfoEXT *>(&createInfo),
          nullptr,
          reinterpret_cast<VkDebugReportCallbackEXT *>(&this->callback_)) !=
      VK_SUCCESS) {

    throw std::runtime_error("Failed to setup debug callback");
  }
}

bool Context::checkPhysicalDeviceProperties(
    vk::PhysicalDevice physicalDevice,
    vk::SurfaceKHR &surface,
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
      fstl::log::warn(
          "Physical device {} doesn't support extension named \"{}\"",
          physicalDevice.getProperties().deviceName,
          requiredExtension);
      return false;
    }
  }

  auto deviceProperties = physicalDevice.getProperties();
  // auto deviceFeatures = physicalDevice.getFeatures();

  uint32_t majorVersion = VK_VERSION_MAJOR(deviceProperties.apiVersion);
  // uint32_t minorVersion = VK_VERSION_MINOR(deviceProperties.apiVersion);
  // uint32_t patchVersion = VK_VERSION_PATCH(deviceProperties.apiVersion);

  if (majorVersion < 1 && deviceProperties.limits.maxImageDimension2D < 4096) {
    fstl::log::warn(
        "Physical device {} doesn't support required parameters!",
        physicalDevice.getProperties().deviceName);
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
    queuePresentSupport[i] = physicalDevice.getSurfaceSupportKHR(i, surface);

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
    fstl::log::warn(
        "Could not find queue family with requested properties on physical "
        "device {}",
        physicalDevice.getProperties().deviceName);
    return false;
  }

  *graphicsQueueFamily = graphicsQueueFamilyIndex;
  *presentQueueFamily = presentQueueFamilyIndex;
  *transferQueueFamily = transferQueueFamilyIndex;

  return true;
}
