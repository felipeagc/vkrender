#include "context.hpp"
#include "util.hpp"
#include "window.hpp"
#include <ftl/logging.hpp>

re_context_t g_ctx;

// Debug callback

// Ignore warnings for this function
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(
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

static inline bool check_validation_layer_support() {
  uint32_t count;
  vkEnumerateInstanceLayerProperties(&count, nullptr);
  ftl::small_vector<VkLayerProperties> availableLayers(count);
  vkEnumerateInstanceLayerProperties(&count, availableLayers.data());

  for (const char *layerName : RE_REQUIRED_VALIDATION_LAYERS) {
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

static inline std::vector<const char *>
get_required_extensions(std::vector<const char *> sdlExtensions) {
  std::vector<const char *> extensions{sdlExtensions};

#ifdef RE_ENABLE_VALIDATION
  extensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
#endif

  return extensions;
}

static inline bool check_physical_device_properties(
    VkPhysicalDevice physicalDevice,
    VkSurfaceKHR &surface,
    uint32_t *graphicsQueueFamily,
    uint32_t *presentQueueFamily,
    uint32_t *transferQueueFamily) {
  uint32_t count;
  vkEnumerateDeviceExtensionProperties(
      physicalDevice, nullptr, &count, nullptr);
  ftl::small_vector<VkExtensionProperties> availableExtensions(count);
  vkEnumerateDeviceExtensionProperties(
      physicalDevice, nullptr, &count, availableExtensions.data());

  VkPhysicalDeviceProperties deviceProperties;
  vkGetPhysicalDeviceProperties(physicalDevice, &deviceProperties);

  for (const auto &requiredExtension : RE_REQUIRED_DEVICE_EXTENSIONS) {
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
  ftl::small_vector<VkQueueFamilyProperties> queueFamilyProperties(count);
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

static inline void create_instance(
    re_context_t *ctx,
    const std::vector<const char *> &requiredWindowVulkanExtensions) {
  ftl::debug("Creating vulkan instance");
#ifdef RE_ENABLE_VALIDATION
  ftl::debug("Using validation layers");
  if (!check_validation_layer_support()) {
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

#ifdef RE_ENABLE_VALIDATION
  createInfo.enabledLayerCount =
      (uint32_t)ARRAYSIZE(RE_REQUIRED_VALIDATION_LAYERS);
  createInfo.ppEnabledLayerNames = RE_REQUIRED_VALIDATION_LAYERS;
#else
  createInfo.enabledLayerCount = 0;
  createInfo.ppEnabledLayerNames = nullptr;
#endif

  // Set required instance extensions
  auto extensions = get_required_extensions(requiredWindowVulkanExtensions);
  createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
  createInfo.ppEnabledExtensionNames = extensions.data();

  VK_CHECK(vkCreateInstance(&createInfo, nullptr, &ctx->instance));
}

static inline void setup_debug_callback(re_context_t *ctx) {
  VkDebugReportCallbackCreateInfoEXT createInfo = {};
  createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
  createInfo.flags =
      VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
  createInfo.pfnCallback = debug_callback;

  VK_CHECK(CreateDebugReportCallbackEXT(
      ctx->instance, &createInfo, nullptr, &ctx->debug_callback));
}

static inline void create_device(re_context_t *ctx, VkSurfaceKHR *surface) {
  uint32_t count;
  vkEnumeratePhysicalDevices(ctx->instance, &count, nullptr);
  ftl::small_vector<VkPhysicalDevice> physicalDevices(count);
  vkEnumeratePhysicalDevices(ctx->instance, &count, physicalDevices.data());

  for (auto physical_device : physicalDevices) {
    if (check_physical_device_properties(
            physical_device,
            *surface,
            &ctx->graphics_queue_family_index,
            &ctx->present_queue_family_index,
            &ctx->transfer_queue_family_index)) {
      ctx->physical_device = physical_device;
      break;
    }
  }

  if (!ctx->physical_device) {
    throw std::runtime_error(
        "Could not select physical device based on chosen properties");
  }

  VkPhysicalDeviceProperties properties;
  vkGetPhysicalDeviceProperties(ctx->physical_device, &properties);

  ftl::debug("Using physical device: %s", properties.deviceName);

  ftl::small_vector<VkDeviceQueueCreateInfo> queueCreateInfos;
  float queuePriorities[] = {1.0f};

  queueCreateInfos.push_back({
      VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
      nullptr,
      0,
      ctx->graphics_queue_family_index,
      static_cast<uint32_t>(ARRAYSIZE(queuePriorities)),
      queuePriorities,
  });

  if (ctx->present_queue_family_index != ctx->graphics_queue_family_index) {
    queueCreateInfos.push_back({
        VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        nullptr,
        0,
        ctx->present_queue_family_index,
        static_cast<uint32_t>(ARRAYSIZE(queuePriorities)),
        queuePriorities,
    });
  }

  if (ctx->transfer_queue_family_index != ctx->graphics_queue_family_index) {
    queueCreateInfos.push_back({
        VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        nullptr,
        0,
        ctx->transfer_queue_family_index,
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
#ifdef RE_ENABLE_VALIDATION
  deviceCreateInfo.enabledLayerCount =
      (uint32_t)ARRAYSIZE(RE_REQUIRED_VALIDATION_LAYERS);
  deviceCreateInfo.ppEnabledLayerNames = RE_REQUIRED_VALIDATION_LAYERS;
#else
  deviceCreateInfo.enabledLayerCount = 0;
  deviceCreateInfo.ppEnabledLayerNames = nullptr;
#endif

  deviceCreateInfo.enabledExtensionCount =
      (uint32_t)ARRAYSIZE(RE_REQUIRED_DEVICE_EXTENSIONS);
  deviceCreateInfo.ppEnabledExtensionNames = RE_REQUIRED_DEVICE_EXTENSIONS;

  // Enable all features
  VkPhysicalDeviceFeatures features;
  vkGetPhysicalDeviceFeatures(ctx->physical_device, &features);
  deviceCreateInfo.pEnabledFeatures = &features;

  VK_CHECK(vkCreateDevice(
      ctx->physical_device, &deviceCreateInfo, nullptr, &ctx->device));
}

static inline void get_device_queues(re_context_t *ctx) {
  vkGetDeviceQueue(
      ctx->device, ctx->graphics_queue_family_index, 0, &ctx->graphics_queue);
  vkGetDeviceQueue(
      ctx->device, ctx->present_queue_family_index, 0, &ctx->present_queue);
  vkGetDeviceQueue(
      ctx->device, ctx->transfer_queue_family_index, 0, &ctx->transfer_queue);
}

static inline void setup_memory_allocator(re_context_t *ctx) {
  VmaAllocatorCreateInfo allocatorInfo = {};
  allocatorInfo.physicalDevice = ctx->physical_device;
  allocatorInfo.device = ctx->device;

  VK_CHECK(vmaCreateAllocator(&allocatorInfo, &ctx->gpu_allocator));
}

static inline void create_graphics_command_pool(re_context_t *ctx) {
  VkCommandPoolCreateInfo createInfo = {};
  createInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  createInfo.pNext = 0;
  createInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
  createInfo.queueFamilyIndex = ctx->graphics_queue_family_index;

  VK_CHECK(vkCreateCommandPool(
      ctx->device, &createInfo, nullptr, &ctx->graphics_command_pool));
}

static inline void create_transient_command_pool(re_context_t *ctx) {
  VkCommandPoolCreateInfo createInfo = {};
  createInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  createInfo.pNext = 0;
  createInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
  createInfo.queueFamilyIndex = ctx->graphics_queue_family_index;

  VK_CHECK(vkCreateCommandPool(
      ctx->device, &createInfo, nullptr, &ctx->transient_command_pool));
}

static inline void create_thread_command_pools(re_context_t *ctx) {
  for (uint32_t i = 0; i < RE_THREAD_COUNT; i++) {
    VkCommandPoolCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    createInfo.pNext = 0;
    createInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
    createInfo.queueFamilyIndex = ctx->graphics_queue_family_index;

    VK_CHECK(vkCreateCommandPool(
        ctx->device, &createInfo, nullptr, &ctx->thread_command_pools[i]));
  }
}

void re_context_pre_init(
    re_context_t *ctx,
    const std::vector<const char *> &required_window_vulkan_extensions) {
  ctx->instance = VK_NULL_HANDLE;
  ctx->device = VK_NULL_HANDLE;
  ctx->physical_device = VK_NULL_HANDLE;
  ctx->debug_callback = VK_NULL_HANDLE;

  ctx->graphics_queue_family_index = -1;
  ctx->present_queue_family_index = -1;
  ctx->transfer_queue_family_index = -1;
  ctx->graphics_queue = VK_NULL_HANDLE;
  ctx->present_queue = VK_NULL_HANDLE;
  ctx->transfer_queue = VK_NULL_HANDLE;

  ctx->gpu_allocator = VK_NULL_HANDLE;
  ctx->graphics_command_pool = VK_NULL_HANDLE;
  ctx->transient_command_pool = VK_NULL_HANDLE;

  for (uint32_t i = 0; i < ARRAYSIZE(ctx->thread_command_pools); i++) {
    ctx->thread_command_pools[i] = VK_NULL_HANDLE;
  }

  create_instance(ctx, required_window_vulkan_extensions);
#ifdef RE_ENABLE_VALIDATION
  setup_debug_callback(ctx);
#endif
}

void re_context_late_inint(re_context_t *ctx, VkSurfaceKHR *surface) {
  if (ctx->device != VK_NULL_HANDLE) {
    return;
  }

  create_device(ctx, surface);

  get_device_queues(ctx);

  setup_memory_allocator(ctx);

  create_graphics_command_pool(ctx);
  create_transient_command_pool(ctx);
  create_thread_command_pools(ctx);

  re_resource_manager_init(&ctx->resource_manager);

  uint8_t white[] = {255, 255, 255, 255};
  uint8_t black[] = {0, 0, 0, 255};
  re_texture_init(&ctx->white_texture, white, sizeof(white), 1, 1);
  re_texture_init(&ctx->black_texture, black, sizeof(black), 1, 1);
}

VkSampleCountFlagBits re_context_get_max_sample_count(re_context_t *ctx) {
  VkPhysicalDeviceProperties properties;
  vkGetPhysicalDeviceProperties(ctx->physical_device, &properties);

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

bool re_context_get_supported_depth_format(
    re_context_t *ctx, VkFormat *depthFormat) {
  VkFormat depthFormats[] = {VK_FORMAT_D32_SFLOAT_S8_UINT,
                             VK_FORMAT_D32_SFLOAT,
                             VK_FORMAT_D24_UNORM_S8_UINT,
                             VK_FORMAT_D16_UNORM_S8_UINT,
                             VK_FORMAT_D16_UNORM};
  for (auto &format : depthFormats) {
    VkFormatProperties formatProps;
    vkGetPhysicalDeviceFormatProperties(
        ctx->physical_device, format, &formatProps);

    if (formatProps.optimalTilingFeatures &
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) {
      *depthFormat = format;
      return format;
      return true;
    }
  }
  return false;
}

void re_context_destroy(re_context_t *ctx) {
  ftl::debug("Vulkan context shutting down...");

  VK_CHECK(vkDeviceWaitIdle(ctx->device));

  re_texture_destroy(&ctx->white_texture);
  re_texture_destroy(&ctx->black_texture);

  re_resource_manager_destroy(&ctx->resource_manager);

  vkDestroyCommandPool(ctx->device, ctx->transient_command_pool, nullptr);

  vkDestroyCommandPool(ctx->device, ctx->graphics_command_pool, nullptr);

  for (auto &command_pool : ctx->thread_command_pools) {
    vkDestroyCommandPool(ctx->device, command_pool, nullptr);
  }

  vmaDestroyAllocator(ctx->gpu_allocator);

  vkDestroyDevice(ctx->device, nullptr);

  DestroyDebugReportCallbackEXT(ctx->instance, ctx->debug_callback, nullptr);

  vkDestroyInstance(ctx->instance, nullptr);

  SDL_Quit();
}
