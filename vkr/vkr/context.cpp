#include "context.hpp"
#include "util.hpp"
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

namespace vkr {
namespace ctx {
VkInstance instance = VK_NULL_HANDLE;
VkDevice device = VK_NULL_HANDLE;
VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;

VkDebugReportCallbackEXT callback = VK_NULL_HANDLE;

uint32_t graphicsQueueFamilyIndex = VK_NULL_HANDLE;
uint32_t presentQueueFamilyIndex = VK_NULL_HANDLE;
uint32_t transferQueueFamilyIndex = VK_NULL_HANDLE;

VkQueue graphicsQueue = VK_NULL_HANDLE;
VkQueue presentQueue = VK_NULL_HANDLE;
VkQueue transferQueue = VK_NULL_HANDLE;

VmaAllocator allocator = VK_NULL_HANDLE;

VkCommandPool graphicsCommandPool = VK_NULL_HANDLE;
VkCommandPool transientCommandPool = VK_NULL_HANDLE;

DescriptorManager descriptorManager;

static bool checkValidationLayerSupport() {
  uint32_t count;
  vkEnumerateInstanceLayerProperties(&count, nullptr);
  fstl::fixed_vector<VkLayerProperties> availableLayers(count);
  vkEnumerateInstanceLayerProperties(&count, availableLayers.data());

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

static std::vector<const char *>
getRequiredExtensions(std::vector<const char *> sdlExtensions) {
  std::vector<const char *> extensions{sdlExtensions};

#ifndef NDEBUG
  extensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
#endif

  return extensions;
}

static bool checkPhysicalDeviceProperties(
    VkPhysicalDevice physicalDevice,
    VkSurfaceKHR &surface,
    uint32_t *graphicsQueueFamily,
    uint32_t *presentQueueFamily,
    uint32_t *transferQueueFamily) {
  uint32_t count;
  vkEnumerateDeviceExtensionProperties(
      physicalDevice, nullptr, &count, nullptr);
  fstl::fixed_vector<VkExtensionProperties> availableExtensions(count);
  vkEnumerateDeviceExtensionProperties(
      physicalDevice, nullptr, &count, availableExtensions.data());

  VkPhysicalDeviceProperties deviceProperties;
  vkGetPhysicalDeviceProperties(physicalDevice, &deviceProperties);

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
          deviceProperties.deviceName,
          requiredExtension);
      return false;
    }
  }

  uint32_t majorVersion = VK_VERSION_MAJOR(deviceProperties.apiVersion);
  // uint32_t minorVersion = VK_VERSION_MINOR(deviceProperties.apiVersion);
  // uint32_t patchVersion = VK_VERSION_PATCH(deviceProperties.apiVersion);

  if (majorVersion < 1 && deviceProperties.limits.maxImageDimension2D < 4096) {
    fstl::log::warn(
        "Physical device {} doesn't support required parameters!",
        deviceProperties.deviceName);
    return false;
  }

  vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &count, nullptr);
  fstl::fixed_vector<VkQueueFamilyProperties> queueFamilyProperties(count);
  vkGetPhysicalDeviceQueueFamilyProperties(
      physicalDevice, &count, queueFamilyProperties.data());

  std::vector<VkBool32> queuePresentSupport(queueFamilyProperties.size());
  std::vector<VkBool32> queueTransferSupport(queueFamilyProperties.size());

  uint32_t graphicsQueueFamilyIndex = UINT32_MAX;
  uint32_t presentQueueFamilyIndex = UINT32_MAX;
  uint32_t transferQueueFamilyIndex = UINT32_MAX;

  // TODO: figure out better logic to single out a transfer queue

  for (uint32_t i = 0; i < queueFamilyProperties.size(); i++) {
    VK_CHECK(vkGetPhysicalDeviceSurfaceSupportKHR(
        physicalDevice, i, surface, &queuePresentSupport[i]));

    if (queueFamilyProperties[i].queueCount > 0 &&
        queueFamilyProperties[i].queueFlags & VK_QUEUE_TRANSFER_BIT &&
        !(queueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)) {
      transferQueueFamilyIndex = i;
    }

    if (queueFamilyProperties[i].queueCount > 0 &&
        queueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
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

  fstl::log::info("Graphics: {}", graphicsQueueFamilyIndex);
  fstl::log::info("Present: {}", presentQueueFamilyIndex);
  fstl::log::info("Transfer: {}", transferQueueFamilyIndex);

  if (graphicsQueueFamilyIndex == UINT32_MAX ||
      presentQueueFamilyIndex == UINT32_MAX ||
      transferQueueFamilyIndex == UINT32_MAX) {
    fstl::log::warn(
        "Could not find queue family with requested properties on physical "
        "device {}",
        deviceProperties.deviceName);
    return false;
  }

  *graphicsQueueFamily = graphicsQueueFamilyIndex;
  *presentQueueFamily = presentQueueFamilyIndex;
  *transferQueueFamily = transferQueueFamilyIndex;

  return true;
}

static void createInstance(
    const std::vector<const char *> &requiredWindowVulkanExtensions) {
  fstl::log::debug("Creating vulkan instance");
#ifndef NDEBUG
  fstl::log::debug("Using validation layers");
  if (!checkValidationLayerSupport()) {
    throw std::runtime_error("Validation layers requested, but not available");
  }
#else
  fstl::log::debug("Not using validation layers");
#endif

  VkApplicationInfo appInfo = {};
  appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  appInfo.pNext = nullptr;
  appInfo.pApplicationName = "App";
  appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
  appInfo.pEngineName = "No engine";
  appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
  appInfo.apiVersion = VK_API_VERSION_1_0;

  VkInstanceCreateInfo createInfo = {};
  createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  createInfo.flags = 0;
  createInfo.pApplicationInfo = &appInfo;

#ifdef NDEBUG
  createInfo.enabledLayerCount = 0;
  createInfo.ppEnabledLayerNames = nullptr;
#else
  createInfo.enabledLayerCount =
      static_cast<uint32_t>(REQUIRED_VALIDATION_LAYERS.size());
  createInfo.ppEnabledLayerNames = REQUIRED_VALIDATION_LAYERS.data();
#endif

  // Set required instance extensions
  auto extensions = getRequiredExtensions(requiredWindowVulkanExtensions);
  createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
  createInfo.ppEnabledExtensionNames = extensions.data();

  VK_CHECK(vkCreateInstance(&createInfo, nullptr, &instance));
}

static void setupDebugCallback() {
  VkDebugReportCallbackCreateInfoEXT createInfo = {};
  createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
  createInfo.flags =
      VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
  createInfo.pfnCallback = debugCallback;

  VK_CHECK(
      CreateDebugReportCallbackEXT(instance, &createInfo, nullptr, &callback));
}

static void createDevice(VkSurfaceKHR &surface) {
  uint32_t count;
  vkEnumeratePhysicalDevices(instance, &count, nullptr);
  fstl::fixed_vector<VkPhysicalDevice> physicalDevices(count);
  vkEnumeratePhysicalDevices(instance, &count, physicalDevices.data());

  for (auto physicalDevice_ : physicalDevices) {
    if (checkPhysicalDeviceProperties(
            physicalDevice_,
            surface,
            &graphicsQueueFamilyIndex,
            &presentQueueFamilyIndex,
            &transferQueueFamilyIndex)) {
      physicalDevice = physicalDevice_;
      break;
    }
  }

  if (!physicalDevice) {
    throw std::runtime_error(
        "Could not select physical device based on chosen properties");
  }

  VkPhysicalDeviceProperties properties;
  vkGetPhysicalDeviceProperties(physicalDevice, &properties);

  fstl::log::debug("Using physical device: {}", properties.deviceName);

  fstl::fixed_vector<VkDeviceQueueCreateInfo> queueCreateInfos;
  std::array<float, 1> queuePriorities = {1.0f};

  queueCreateInfos.push_back({
      VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
      nullptr,
      0,
      graphicsQueueFamilyIndex,
      static_cast<uint32_t>(queuePriorities.size()),
      queuePriorities.data(),
  });

  if (presentQueueFamilyIndex != graphicsQueueFamilyIndex) {
    queueCreateInfos.push_back({
        VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        nullptr,
        0,
        presentQueueFamilyIndex,
        static_cast<uint32_t>(queuePriorities.size()),
        queuePriorities.data(),
    });
  }

  if (transferQueueFamilyIndex != graphicsQueueFamilyIndex) {
    queueCreateInfos.push_back({
        VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        nullptr,
        0,
        transferQueueFamilyIndex,
        static_cast<uint32_t>(queuePriorities.size()),
        queuePriorities.data(),
    });
  }

  VkDeviceCreateInfo deviceCreateInfo = {};
  deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  deviceCreateInfo.flags = 0;
  deviceCreateInfo.queueCreateInfoCount =
      static_cast<uint32_t>(queueCreateInfos.size());
  deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();

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
  VkPhysicalDeviceFeatures features;
  vkGetPhysicalDeviceFeatures(physicalDevice, &features);
  deviceCreateInfo.pEnabledFeatures = &features;

  VK_CHECK(vkCreateDevice(physicalDevice, &deviceCreateInfo, nullptr, &device));
}

static void getDeviceQueues() {
  vkGetDeviceQueue(device, graphicsQueueFamilyIndex, 0, &graphicsQueue);
  vkGetDeviceQueue(device, presentQueueFamilyIndex, 0, &presentQueue);
  vkGetDeviceQueue(device, transferQueueFamilyIndex, 0, &transferQueue);
}

static void setupMemoryAllocator() {
  VmaAllocatorCreateInfo allocatorInfo = {};
  allocatorInfo.physicalDevice = physicalDevice;
  allocatorInfo.device = device;

  VK_CHECK(vmaCreateAllocator(&allocatorInfo, &allocator));
}

static void createGraphicsCommandPool() {
  VkCommandPoolCreateInfo createInfo = {};
  createInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  createInfo.pNext = 0;
  createInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
  createInfo.queueFamilyIndex = graphicsQueueFamilyIndex;

  VK_CHECK(
      vkCreateCommandPool(device, &createInfo, nullptr, &graphicsCommandPool));
}

static void createTransientCommandPool() {
  VkCommandPoolCreateInfo createInfo = {};
  createInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  createInfo.pNext = 0;
  createInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
  createInfo.queueFamilyIndex = graphicsQueueFamilyIndex;

  VK_CHECK(
      vkCreateCommandPool(device, &createInfo, nullptr, &transientCommandPool));
}

void preInitialize(
    const std::vector<const char *> &requiredWindowVulkanExtensions) {
  if (instance != VK_NULL_HANDLE) {
    return;
  }

  createInstance(requiredWindowVulkanExtensions);
#ifndef NDEBUG
  setupDebugCallback();
#endif
}

void lateInitialize(VkSurfaceKHR &surface) {
  if (device != VK_NULL_HANDLE) {
    return;
  }

  createDevice(surface);

  getDeviceQueues();

  setupMemoryAllocator();

  createGraphicsCommandPool();
  createTransientCommandPool();

  descriptorManager.init();
}

void destroy() {
  fstl::log::debug("Vulkan context shutting down...");

  VK_CHECK(vkDeviceWaitIdle(device));

  descriptorManager.destroy();

  if (transientCommandPool) {
    vkDestroyCommandPool(device, transientCommandPool, nullptr);
  }

  if (graphicsCommandPool) {
    vkDestroyCommandPool(device, graphicsCommandPool, nullptr);
  }

  if (allocator != VK_NULL_HANDLE) {
    vmaDestroyAllocator(allocator);
  }

  if (device != VK_NULL_HANDLE) {
    vkDestroyDevice(device, nullptr);
  }

  if (callback != VK_NULL_HANDLE) {
    DestroyDebugReportCallbackEXT(instance, callback, nullptr);
  }

  if (instance != VK_NULL_HANDLE) {
    vkDestroyInstance(instance, nullptr);
  }

  SDL_Quit();
}

VkSampleCountFlagBits getMaxUsableSampleCount() {
  VkPhysicalDeviceProperties properties;
  vkGetPhysicalDeviceProperties(physicalDevice, &properties);

  VkSampleCountFlags colorSamples =
      properties.limits.framebufferColorSampleCounts;
  VkSampleCountFlags depthSamples =
      properties.limits.framebufferDepthSampleCounts;

  VkSampleCountFlags counts = std::min(colorSamples, depthSamples);

  if (counts & VK_SAMPLE_COUNT_64_BIT) {
    fstl::log::debug("Max samples: {}", 64);
    return VK_SAMPLE_COUNT_64_BIT;
  }
  if (counts & VK_SAMPLE_COUNT_32_BIT) {
    fstl::log::debug("Max samples: {}", 32);
    return VK_SAMPLE_COUNT_32_BIT;
  }
  if (counts & VK_SAMPLE_COUNT_16_BIT) {
    fstl::log::debug("Max samples: {}", 16);
    return VK_SAMPLE_COUNT_16_BIT;
  }
  if (counts & VK_SAMPLE_COUNT_8_BIT) {
    fstl::log::debug("Max samples: {}", 8);
    return VK_SAMPLE_COUNT_8_BIT;
  }
  if (counts & VK_SAMPLE_COUNT_4_BIT) {
    fstl::log::debug("Max samples: {}", 4);
    return VK_SAMPLE_COUNT_4_BIT;
  }
  if (counts & VK_SAMPLE_COUNT_2_BIT) {
    fstl::log::debug("Max samples: {}", 2);
    return VK_SAMPLE_COUNT_2_BIT;
  }

  fstl::log::debug("Max samples: {}", 1);

  return VK_SAMPLE_COUNT_1_BIT;
}
} // namespace ctx
} // namespace vkr
