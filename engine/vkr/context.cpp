#include "context.hpp"
#include "window.hpp"
#include <cstdio>
#include <iostream>

using namespace vkr;

// Debug callback stuff

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugReportFlagsEXT flags,
    VkDebugReportObjectTypeEXT objType,
    uint64_t obj,
    size_t location,
    int32_t code,
    const char *layerPrefix,
    const char *msg,
    void *userData) {
  printf("Validation layer: %s\n", msg);

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

Context::Context(const Window &window) {
  this->createInstance(window.getVulkanExtensions());
#ifndef NDEBUG
  this->setupDebugCallback();
#endif
  this->surface = window.createVulkanSurface(this->instance);

  this->createDevice();

  // TODO:
  // this->getDeviceQueues();
  // this->setupMemoryAllocator();

  // this->createSyncObjects();

  // this->createSwapchain(window->getWidth(), window->getHeight());
  // this->createSwapchainImageViews();

  // this->createGraphicsCommandPool();
  // this->createTransientCommandPool();
  // this->allocateGraphicsCommandBuffers();

  // this->createDepthResources();

  // this->createRenderPass();
}

Context::~Context() {
  this->device.destroy();
  instance.destroy(this->surface);

  if (this->callback) {
    DestroyDebugReportCallbackEXT(this->instance, this->callback, nullptr);
  }

  instance.destroy();
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

  // Selected queue indices
  uint32_t graphicsQueue = UINT32_MAX;
  uint32_t presentQueue = UINT32_MAX;
  uint32_t transferQueue = UINT32_MAX;

  for (auto physicalDevice : physicalDevices) {
    if (checkPhysicalDeviceProperties(
            physicalDevice, &graphicsQueue, &presentQueue, &transferQueue)) {
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
      graphicsQueue,
      static_cast<uint32_t>(queuePriorities.size()),
      queuePriorities.data(),
  });

  if (presentQueue != graphicsQueue) {
    queueCreateInfos.push_back({
        {},
        presentQueue,
        static_cast<uint32_t>(queuePriorities.size()),
        queuePriorities.data(),
    });
  }

  if (transferQueue != graphicsQueue) {
    queueCreateInfos.push_back({
        {},
        transferQueue,
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
  uint32_t layerCount;
  vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

  std::vector<VkLayerProperties> availableLayers(layerCount);
  vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

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
    uint32_t *graphicsQueue,
    uint32_t *presentQueue,
    uint32_t *transferQueue) {
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
          *transferQueue = i;
        }
        *graphicsQueue = i;
        *presentQueue = i;
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

  *graphicsQueue = graphicsQueueFamilyIndex;
  *presentQueue = presentQueueFamilyIndex;
  *transferQueue = transferQueueFamilyIndex;

  return true;
}
