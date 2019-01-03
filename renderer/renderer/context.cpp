#include "context.hpp"
#include "util.hpp"
#include "window.hpp"
#include <ftl/logging.hpp>

using namespace renderer;

static Context *globalContext;

Context &renderer::ctx() { return *globalContext; }

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
  ftl::error("Validation layer: %s", msg);

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

static bool checkValidationLayerSupport() {
  uint32_t count;
  vkEnumerateInstanceLayerProperties(&count, nullptr);
  ftl::fixed_vector<VkLayerProperties> availableLayers(count);
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

#ifdef VKR_ENABLE_VALIDATION
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
  ftl::fixed_vector<VkExtensionProperties> availableExtensions(count);
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
      ftl::warn(
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
    ftl::warn(
        "Physical device {} doesn't support required parameters!",
        deviceProperties.deviceName);
    return false;
  }

  // Check for required device features
  VkPhysicalDeviceFeatures features = {};
  vkGetPhysicalDeviceFeatures(physicalDevice, &features);
  if (!features.wideLines) {
    ftl::warn(
        "Physical device {} doesn't support required features!",
        deviceProperties.deviceName);
    return false;
  }

  vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &count, nullptr);
  ftl::fixed_vector<VkQueueFamilyProperties> queueFamilyProperties(count);
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

  if (graphicsQueueFamilyIndex == UINT32_MAX ||
      presentQueueFamilyIndex == UINT32_MAX ||
      transferQueueFamilyIndex == UINT32_MAX) {
    ftl::warn(
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

Context::Context() { globalContext = this; }

void Context::createInstance(
    const std::vector<const char *> &requiredWindowVulkanExtensions) {
  ftl::debug("Creating vulkan instance");
#ifdef VKR_ENABLE_VALIDATION
  ftl::debug("Using validation layers");
  if (!checkValidationLayerSupport()) {
    throw std::runtime_error("Validation layers requested, but not available");
  }
#else
  ftl::debug("Not using validation layers");
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

#ifdef VKR_ENABLE_VALIDATION
  createInfo.enabledLayerCount =
      static_cast<uint32_t>(REQUIRED_VALIDATION_LAYERS.size());
  createInfo.ppEnabledLayerNames = REQUIRED_VALIDATION_LAYERS.data();
#else
  createInfo.enabledLayerCount = 0;
  createInfo.ppEnabledLayerNames = nullptr;
#endif

  // Set required instance extensions
  auto extensions = getRequiredExtensions(requiredWindowVulkanExtensions);
  createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
  createInfo.ppEnabledExtensionNames = extensions.data();

  VK_CHECK(vkCreateInstance(&createInfo, nullptr, &m_instance));
}

void Context::setupDebugCallback() {
  VkDebugReportCallbackCreateInfoEXT createInfo = {};
  createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
  createInfo.flags =
      VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
  createInfo.pfnCallback = debugCallback;

  VK_CHECK(CreateDebugReportCallbackEXT(
      m_instance, &createInfo, nullptr, &m_callback));
}

void Context::createDevice(VkSurfaceKHR &surface) {
  uint32_t count;
  vkEnumeratePhysicalDevices(m_instance, &count, nullptr);
  ftl::fixed_vector<VkPhysicalDevice> physicalDevices(count);
  vkEnumeratePhysicalDevices(m_instance, &count, physicalDevices.data());

  for (auto physicalDevice_ : physicalDevices) {
    if (checkPhysicalDeviceProperties(
            physicalDevice_,
            surface,
            &m_graphicsQueueFamilyIndex,
            &m_presentQueueFamilyIndex,
            &m_transferQueueFamilyIndex)) {
      m_physicalDevice = physicalDevice_;
      break;
    }
  }

  if (!m_physicalDevice) {
    throw std::runtime_error(
        "Could not select physical device based on chosen properties");
  }

  VkPhysicalDeviceProperties properties;
  vkGetPhysicalDeviceProperties(m_physicalDevice, &properties);

  ftl::debug("Using physical device: %s", properties.deviceName);

  ftl::fixed_vector<VkDeviceQueueCreateInfo> queueCreateInfos;
  float queuePriorities[] = {1.0f};

  queueCreateInfos.push_back({
      VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
      nullptr,
      0,
      m_graphicsQueueFamilyIndex,
      static_cast<uint32_t>(ARRAYSIZE(queuePriorities)),
      queuePriorities,
  });

  if (m_presentQueueFamilyIndex != m_graphicsQueueFamilyIndex) {
    queueCreateInfos.push_back({
        VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        nullptr,
        0,
        m_presentQueueFamilyIndex,
        static_cast<uint32_t>(ARRAYSIZE(queuePriorities)),
        queuePriorities,
    });
  }

  if (m_transferQueueFamilyIndex != m_graphicsQueueFamilyIndex) {
    queueCreateInfos.push_back({
        VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        nullptr,
        0,
        m_transferQueueFamilyIndex,
        static_cast<uint32_t>(ARRAYSIZE(queuePriorities)),
        queuePriorities,
    });
  }

  VkDeviceCreateInfo deviceCreateInfo = {};
  deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  deviceCreateInfo.flags = 0;
  deviceCreateInfo.queueCreateInfoCount =
      static_cast<uint32_t>(queueCreateInfos.size());
  deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();

  // Validation layer stuff
#ifdef VKR_ENABLE_VALIDATION
  deviceCreateInfo.enabledLayerCount =
      static_cast<uint32_t>(REQUIRED_VALIDATION_LAYERS.size());
  deviceCreateInfo.ppEnabledLayerNames = REQUIRED_VALIDATION_LAYERS.data();
#else
  deviceCreateInfo.enabledLayerCount = 0;
  deviceCreateInfo.ppEnabledLayerNames = nullptr;
#endif

  deviceCreateInfo.enabledExtensionCount =
      static_cast<uint32_t>(REQUIRED_DEVICE_EXTENSIONS.size());
  deviceCreateInfo.ppEnabledExtensionNames = REQUIRED_DEVICE_EXTENSIONS.data();

  // Enable all features
  VkPhysicalDeviceFeatures features;
  vkGetPhysicalDeviceFeatures(m_physicalDevice, &features);
  deviceCreateInfo.pEnabledFeatures = &features;

  VK_CHECK(
      vkCreateDevice(m_physicalDevice, &deviceCreateInfo, nullptr, &m_device));
}

void Context::getDeviceQueues() {
  vkGetDeviceQueue(m_device, m_graphicsQueueFamilyIndex, 0, &m_graphicsQueue);
  vkGetDeviceQueue(m_device, m_presentQueueFamilyIndex, 0, &m_presentQueue);
  vkGetDeviceQueue(m_device, m_transferQueueFamilyIndex, 0, &m_transferQueue);
}

void Context::setupMemoryAllocator() {
  VmaAllocatorCreateInfo allocatorInfo = {};
  allocatorInfo.physicalDevice = m_physicalDevice;
  allocatorInfo.device = m_device;

  VK_CHECK(vmaCreateAllocator(&allocatorInfo, &m_allocator));
}

void Context::createGraphicsCommandPool() {
  VkCommandPoolCreateInfo createInfo = {};
  createInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  createInfo.pNext = 0;
  createInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
  createInfo.queueFamilyIndex = m_graphicsQueueFamilyIndex;

  VK_CHECK(vkCreateCommandPool(
      m_device, &createInfo, nullptr, &m_graphicsCommandPool));
}

void Context::createTransientCommandPool() {
  VkCommandPoolCreateInfo createInfo = {};
  createInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  createInfo.pNext = 0;
  createInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
  createInfo.queueFamilyIndex = m_graphicsQueueFamilyIndex;

  VK_CHECK(vkCreateCommandPool(
      m_device, &createInfo, nullptr, &m_transientCommandPool));
}

void Context::createThreadCommandPools() {
  for (uint32_t i = 0; i < VKR_THREAD_COUNT; i++) {
    VkCommandPoolCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    createInfo.pNext = 0;
    createInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
    createInfo.queueFamilyIndex = m_graphicsQueueFamilyIndex;

    VK_CHECK(vkCreateCommandPool(
        m_device, &createInfo, nullptr, &m_threadCommandPools[i]));
  }
}

void Context::preInitialize(
    const std::vector<const char *> &requiredWindowVulkanExtensions) {
  if (m_instance != VK_NULL_HANDLE) {
    return;
  }

  createInstance(requiredWindowVulkanExtensions);
#ifdef VKR_ENABLE_VALIDATION
  setupDebugCallback();
#endif
}

void Context::lateInitialize(VkSurfaceKHR &surface) {
  if (m_device != VK_NULL_HANDLE) {
    return;
  }

  this->createDevice(surface);

  this->getDeviceQueues();

  this->setupMemoryAllocator();

  this->createGraphicsCommandPool();
  this->createTransientCommandPool();
  this->createThreadCommandPools();

  m_resourceManager.initialize();

  m_whiteTexture = Texture{{255, 255, 255, 255}, 1, 1};
  m_blackTexture = Texture{{0, 0, 0, 255}, 1, 1};
}

Context::~Context() {
  ftl::debug("Vulkan context shutting down...");

  VK_CHECK(vkDeviceWaitIdle(m_device));

  m_whiteTexture.destroy();
  m_blackTexture.destroy();

  m_resourceManager.destroy();

  if (m_transientCommandPool) {
    vkDestroyCommandPool(m_device, m_transientCommandPool, nullptr);
  }

  if (m_graphicsCommandPool) {
    vkDestroyCommandPool(m_device, m_graphicsCommandPool, nullptr);
  }

  for (auto &commandPool : m_threadCommandPools) {
    vkDestroyCommandPool(m_device, commandPool, nullptr);
  }

  if (m_allocator != VK_NULL_HANDLE) {
    vmaDestroyAllocator(m_allocator);
  }

  if (m_device != VK_NULL_HANDLE) {
    vkDestroyDevice(m_device, nullptr);
  }

  if (m_callback != VK_NULL_HANDLE) {
    DestroyDebugReportCallbackEXT(m_instance, m_callback, nullptr);
  }

  if (m_instance != VK_NULL_HANDLE) {
    vkDestroyInstance(m_instance, nullptr);
  }

  SDL_Quit();
}

VkSampleCountFlagBits Context::getMaxUsableSampleCount() {
  VkPhysicalDeviceProperties properties;
  vkGetPhysicalDeviceProperties(m_physicalDevice, &properties);

  VkSampleCountFlags colorSamples =
      properties.limits.framebufferColorSampleCounts;
  VkSampleCountFlags depthSamples =
      properties.limits.framebufferDepthSampleCounts;

  VkSampleCountFlags counts = std::min(colorSamples, depthSamples);

  if (counts & VK_SAMPLE_COUNT_64_BIT) {
    ftl::debug("Max samples: %d", 64);
    return VK_SAMPLE_COUNT_64_BIT;
  }
  if (counts & VK_SAMPLE_COUNT_32_BIT) {
    ftl::debug("Max samples: %d", 32);
    return VK_SAMPLE_COUNT_32_BIT;
  }
  if (counts & VK_SAMPLE_COUNT_16_BIT) {
    ftl::debug("Max samples: %d", 16);
    return VK_SAMPLE_COUNT_16_BIT;
  }
  if (counts & VK_SAMPLE_COUNT_8_BIT) {
    ftl::debug("Max samples: %d", 8);
    return VK_SAMPLE_COUNT_8_BIT;
  }
  if (counts & VK_SAMPLE_COUNT_4_BIT) {
    ftl::debug("Max samples: %d", 4);
    return VK_SAMPLE_COUNT_4_BIT;
  }
  if (counts & VK_SAMPLE_COUNT_2_BIT) {
    ftl::debug("Max samples: %d", 2);
    return VK_SAMPLE_COUNT_2_BIT;
  }

  ftl::debug("Max samples: %d", 1);

  return VK_SAMPLE_COUNT_1_BIT;
}

bool Context::getSupportedDepthFormat(VkFormat *depthFormat) {
  VkFormat depthFormats[] = {VK_FORMAT_D32_SFLOAT_S8_UINT,
                             VK_FORMAT_D32_SFLOAT,
                             VK_FORMAT_D24_UNORM_S8_UINT,
                             VK_FORMAT_D16_UNORM_S8_UINT,
                             VK_FORMAT_D16_UNORM};
  for (auto &format : depthFormats) {
    VkFormatProperties formatProps;
    vkGetPhysicalDeviceFormatProperties(
        ctx().m_physicalDevice, format, &formatProps);

    if (formatProps.optimalTilingFeatures &
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) {
      *depthFormat = format;
      return format;
      return true;
    }
  }
  return false;
}
